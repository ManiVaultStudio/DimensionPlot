#include "DimensionPlotView.h"

#include <event/Event.h>

#include <DatasetsMimeData.h>

#include <util/Timer.h>
#include <SelectionGroup.h>

#include <QHash>
#include <QDebug>
#include <QMimeData>
#include <fstream>
#include <sstream>

Q_PLUGIN_METADATA(IID "studio.manivault.DimensionPlotView")

using namespace mv;

namespace
{
    bool isMetadata(mv::Dataset<DatasetImpl> dataset)
    {
        return dataset->hasProperty("PatchSeqType") && dataset->getProperty("PatchSeqType").toString() == "Metadata";
    }

    QHash<QString, int> mapStringsToIds(const std::vector<QString>& input)
    {
        QHash<QString, int> mapping;
        int nextId = 0;

        for (const QString& str : input) {
            if (!mapping.contains(str)) {
                mapping.insert(str, nextId++);
            }
        }

        return mapping;
    }

    // FIXME stupid method
    QStringList orderedCategoryList(const QHash<QString, int>& categoryToId)
    {
        QVector<QPair<int, QString>> idCategoryPairs;
        for (auto it = categoryToId.constBegin(); it != categoryToId.constEnd(); ++it) {
            idCategoryPairs.append(qMakePair(it.value(), it.key()));
        }

        std::sort(idCategoryPairs.begin(), idCategoryPairs.end(),
            [](const QPair<int, QString>& a, const QPair<int, QString>& b) {
                return a.first < b.first;
            });

        QStringList result;
        for (const auto& pair : idCategoryPairs) {
            result.append(pair.second);
        }

        return result;
    }
}

DimensionPlotView::DimensionPlotView(const PluginFactory* factory) :
    ViewPlugin(factory),
    _dropWidget(nullptr),
    _webWidget(new PlotWebWidget(this)),
    _primaryToolbarAction(this, "PrimaryToolbar"),
    _settingsAction(this, "SettingsAction")
{

}

void DimensionPlotView::init()
{
    // Create layout
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);

    // Load webpage
    _webWidget->setPage(":dimplot/dimplot/plot_view.html", "qrc:/dimplot/dimplot/");
    
    _primaryToolbarAction.addAction(_settingsAction.getDimensionPicker());

    layout->addWidget(_primaryToolbarAction.createWidget(&getWidget()), 1);
    layout->addWidget(_webWidget, 99);

    // Apply the layout
    getWidget().setLayout(layout);

    //// Respond when the name of the dataset in the dataset reference changes
    //connect(&_scene._cellMetadata, &Dataset<Text>::guiNameChanged, this, [this]()
    //{
    //    // Only show the drop indicator when nothing is loaded in the dataset reference
    //    _dropWidget->setShowDropIndicator(_scene._cellMetadata->getGuiName().isEmpty());
    //});

    //connect(&_scene._ephysTraces, &Dataset<Text>::changed, this, [this]() { connect(&_scene._ephysTraces, &Dataset<Text>::dataSelectionChanged, this, &DimensionPlotView::onCellSelectionChanged); });


    // Instantiate new drop widget: See ExampleViewPlugin for details
    _dropWidget = new DropWidget(_webWidget);
    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag data from the hierarchy in this view"));

    _dropWidget->initialize([this](const QMimeData* mimeData) -> DropWidget::DropRegions {

        // A drop widget can contain zero or more drop regions
        DropWidget::DropRegions dropRegions;

        const auto datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);


        if (datasetsMimeData == nullptr)
            return dropRegions;

        if (datasetsMimeData->getDatasets().count() > 1)
            return dropRegions;

        const auto dataset = datasetsMimeData->getDatasets().first();
        const auto datasetGuiName = dataset->text();
        const auto datasetId = dataset->getId();
        const auto dataType = dataset->getDataType();
        const auto dataTypes = DataTypes({ PointType, ClusterType, TextType });

        if (dataTypes.contains(dataType)) {
            if (_currentDataSet.isValid() && datasetId == _currentDataSet->getId()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                if (dataType == PointType)
                {
                    auto candidateDataset = mv::data().getDataset<Points>(datasetId);

                    dropRegions << new DropWidget::DropRegion(this, "Points", QString("Visualize %1").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        _dropWidget->setShowDropIndicator(false);
                        _currentDataSet = candidateDataset;
                    });
                }
                else if (dataType == ClusterType)
                {
                    auto candidateDataset = mv::data().getDataset<Clusters>(datasetId);

                    dropRegions << new DropWidget::DropRegion(this, "Clusters", QString("Add clusters %1").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        _dropWidget->setShowDropIndicator(false);
                        _clusterDataset = candidateDataset;
                    });
                }
            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);
        }

        return dropRegions;
        });

    // Update data when data set changed
    connect(&_currentDataSet, &Dataset<Points>::changed, this, &DimensionPlotView::onDatasetChanged);
    connect(&_clusterDataset, &Dataset<Clusters>::changed, this, &DimensionPlotView::onDatasetChanged);

    connect(_settingsAction.getDimensionPicker(), &DimensionPickerAction::currentDimensionIndexChanged, this, &DimensionPlotView::onDimensionChanged);
}

void DimensionPlotView::onDatasetChanged()
{
    qDebug() << "Weee dataset changed";

    _settingsAction.getDimensionPicker()->setPointsDataset(_currentDataSet);

    onDimensionChanged();

    //// Get associated metadata dataset
    //for (mv::Dataset dataset : mv::data().getAllDatasets())
    //{
    //    if (isMetadata(dataset))
    //        _cellMetadata = dataset;
    //}
}

void DimensionPlotView::onDimensionChanged()
{
    if (!_currentDataSet.isValid() || !_clusterDataset.isValid())
    {
        return;
    }

    // Get dimension from picker action
    int dimensionIndex = _settingsAction.getDimensionPicker()->getCurrentDimensionIndex();

    _webWidget->setData(_currentDataSet, dimensionIndex, _clusterDataset);
}

void DimensionPlotView::onDimensionChanged2()
{
    if (!_currentDataSet.isValid() || !_cellMetadata.isValid())
    {
        return;
    }

    // Get dimension from picker action
    int dimensionIndex = _settingsAction.getDimensionPicker()->getCurrentDimensionIndex();

    // Get the bimaps of feature data and metadata
    mv::KeyBasedSelectionGroup& selGroup = mv::events().getSelectionGroups()[0];
    const BiMap& featuresBiMap = selGroup.getBiMap(_currentDataSet);
    const BiMap& metadataBiMap = selGroup.getBiMap(_cellMetadata);

    // Get selected metadata ids
    std::vector<uint32_t> indices(_currentDataSet->getNumPoints());
    std::iota(indices.begin(), indices.end(), 0);

    std::vector<QString> keys;
    keys = featuresBiMap.getKeysByValues(indices);
    std::vector<uint32_t> metadataIds = metadataBiMap.getValuesByKeys(keys);

    // Get subclass column from metadata
    std::vector<QString> subclasses = _cellMetadata->getColumn("Subclass");

    // Get indices of every category
    QHash<QString, int> categories = mapStringsToIds(subclasses);

    // Get category ids
    std::vector<int> categoryIds(indices.size());
    for (int i = 0; i < categoryIds.size(); i++)
    {
        QString subclass = subclasses[metadataIds[i]];
        //qDebug() << subclass << i;
        categoryIds[i] = categories[subclass];
    }

    QStringList categoryKeys = orderedCategoryList(categories);

    _webWidget->setData(_currentDataSet, dimensionIndex, categoryKeys, categoryIds);
}

ViewPlugin* DimensionPlotViewFactory::produce()
{
    return new DimensionPlotView(this);
}

mv::DataTypes DimensionPlotViewFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;

    // This example analysis plugin is compatible with points datasets
    supportedTypes.append(PointType);

    return supportedTypes;
}

mv::gui::PluginTriggerActions DimensionPlotViewFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this]() -> DimensionPlotView* {
        return dynamic_cast<DimensionPlotView*>(plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<DimensionPlotViewFactory*>(this), this, "Dimension Plot Viewer", "Plot dimensions", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance();
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
