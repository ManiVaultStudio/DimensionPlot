#include "DimensionPlot.h"

#include <event/Event.h>

#include <DatasetsMimeData.h>

#include <util/Timer.h>
#include <SelectionGroup.h>

#include <QHash>
#include <QDebug>
#include <QMimeData>

Q_PLUGIN_METADATA(IID "studio.manivault.DimensionPlot")

using namespace mv;

DimensionPlot::DimensionPlot(const PluginFactory* factory) :
    ViewPlugin(factory),
    _dropWidget(nullptr),
    _webWidget(new PlotWebWidget(this)),
    _primaryToolbarAction(this, "PrimaryToolbar"),
    _settingsAction(this, "SettingsAction")
{

}

void DimensionPlot::init()
{
    // Create layout
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);

    // Load webpage
    _webWidget->setPage(":dimplot/dimplot/plot_view.html", "qrc:/dimplot/dimplot/");
    
    _primaryToolbarAction.addAction(&_settingsAction);

    layout->addWidget(_primaryToolbarAction.createWidget(&getWidget()), 1);
    layout->addWidget(_webWidget, 99);

    // Apply the layout
    getWidget().setLayout(layout);

    // Instantiate new drop widget: See ExampleViewPlugin for details
    _dropWidget = new DropWidget(_webWidget);
    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No feature data loaded", "Drag feature data from the hierarchy to this view"));

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
        const auto dataTypes = DataTypes({ PointType });

        if (dataTypes.contains(dataType))
        {
            if (_featureDataset.isValid() && datasetId == _featureDataset->getId()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else
            {
                auto candidateDataset = mv::data().getDataset<Points>(datasetId);

                dropRegions << new DropWidget::DropRegion(this, "Points", QString("Visualize %1").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                    _dropWidget->setShowDropIndicator(false);
                    _featureDataset = candidateDataset;
                    _settingsAction.getCurrentDatasetAction().setCurrentDataset(_featureDataset);
                    _clusterDataset = nullptr;

                    onFeatureDatasetChanged();
                });
            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);
        }

        return dropRegions;
        });

    // Update data when data set changed
    connect(_settingsAction.getMetadataPicker(), &DatasetPickerAction::currentIndexChanged, this, &DimensionPlot::onClusterDatasetChanged);
    connect(_settingsAction.getDimensionPicker(), &DimensionPickerAction::currentDimensionIndexChanged, this, &DimensionPlot::onDimensionChanged);
    connect(&_settingsAction.getCurrentDatasetAction(), &DatasetPickerAction::datasetPicked, this, [this]() {
        _dropWidget->setShowDropIndicator(false);
        _featureDataset = _settingsAction.getCurrentDatasetAction().getCurrentDataset();
        onFeatureDatasetChanged();
    });
}

mv::Datasets getClusterDatasets(mv::Dataset<Points> featureDataset)
{
    // Obtain the data hierarchy item from the source dataset
    auto* parentItem = dataHierarchy().getItem(featureDataset->getId());

    // Ask for the data hierarchy children of the given dataset
    DataHierarchyItems childrenItems = dataHierarchy().getChildren(*parentItem);

    mv::Datasets clusterDatasets;
    // Iterate over the children and find the ones of type cluster
    for (DataHierarchyItem* item : childrenItems)
    {
        DataType type = item->getDataType();
        if (type == ClusterType)
        {
            Dataset<Clusters> clusterDataset = item->getDataset<Clusters>();
            clusterDatasets.push_back(clusterDataset);
        }
    }
    return clusterDatasets;
}

void DimensionPlot::onFeatureDatasetChanged()
{
    if (!_featureDataset.isValid())
        return;

    _settingsAction.getDimensionPicker()->setPointsDataset(_featureDataset);

    // Set cluster dataset
    mv::Datasets clusterDatasets = getClusterDatasets(_featureDataset);
    _settingsAction.getMetadataPicker()->setDatasets(clusterDatasets);
    int initialSelectionIndex = 0;
    int minClusters = std::numeric_limits<int>::max();
    for (int i = 0; i < clusterDatasets.size(); i++)
    {
        mv::Dataset<Clusters> clusterDataset = clusterDatasets[i];
        if (clusterDataset->getClusters().size() < minClusters)
        {
            initialSelectionIndex = i;
            minClusters = clusterDataset->getClusters().size();
        }
    }
    _settingsAction.getMetadataPicker()->setCurrentIndex(initialSelectionIndex);

    onClusterDatasetChanged();

    onDimensionChanged();
}

void DimensionPlot::onClusterDatasetChanged()
{
    _clusterDataset = _settingsAction.getMetadataPicker()->getCurrentDataset();

    onDimensionChanged();
}

void DimensionPlot::onDimensionChanged()
{
    if (!_featureDataset.isValid() || !_clusterDataset.isValid())
    {
        qWarning() << "[DimensionPlot] No valid metadata dataset set";
        return;
    }

    // Get dimension from picker action
    int dimensionIndex = _settingsAction.getDimensionPicker()->getCurrentDimensionIndex();

    if (dimensionIndex >= 0 && dimensionIndex < _featureDataset->getNumDimensions())
        _webWidget->setData(_featureDataset, dimensionIndex, _clusterDataset);
}

void DimensionPlot::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);

    variantMapMustContain(variantMap, "SettingsAction");

    _settingsAction.fromVariantMap(variantMap["SettingsAction"].toMap());
}

QVariantMap DimensionPlot::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();

    _settingsAction.insertIntoVariantMap(variantMap);

    return variantMap;
}

ViewPlugin* DimensionPlotFactory::produce()
{
    return new DimensionPlot(this);
}

mv::DataTypes DimensionPlotFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;

    // This example analysis plugin is compatible with points datasets
    supportedTypes.append(PointType);

    return supportedTypes;
}

mv::gui::PluginTriggerActions DimensionPlotFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this]() -> DimensionPlot* {
        return dynamic_cast<DimensionPlot*>(plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<DimensionPlotFactory*>(this), this, "Dimension Plot", "Plot dimensions", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance();
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
