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
    
    _primaryToolbarAction.addAction(_settingsAction.getDimensionPicker());

    layout->addWidget(_primaryToolbarAction.createWidget(&getWidget()), 1);
    layout->addWidget(_webWidget, 99);

    // Apply the layout
    getWidget().setLayout(layout);

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
        const auto dataTypes = DataTypes({ PointType, ClusterType });

        if (dataTypes.contains(dataType)) {
            if (_featureDataset.isValid() && datasetId == _featureDataset->getId()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                if (dataType == PointType)
                {
                    auto candidateDataset = mv::data().getDataset<Points>(datasetId);

                    dropRegions << new DropWidget::DropRegion(this, "Points", QString("Visualize %1").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        _dropWidget->setShowDropIndicator(false);
                        _featureDataset = candidateDataset;
                        _clusterDataset = nullptr;

                        onDatasetChanged();
                    });
                }
                else if (dataType == ClusterType)
                {
                    auto candidateDataset = mv::data().getDataset<Clusters>(datasetId);

                    dropRegions << new DropWidget::DropRegion(this, "Clusters", QString("Add clusters %1").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        _dropWidget->setShowDropIndicator(false);
                        _clusterDataset = candidateDataset;

                        onDatasetChanged();
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
    //connect(&_featureDataset, &Dataset<Points>::changed, this, &DimensionPlot::onDatasetChanged);
    connect(&_clusterDataset, &Dataset<Clusters>::changed, this, &DimensionPlot::onDatasetChanged);

    connect(_settingsAction.getDimensionPicker(), &DimensionPickerAction::currentDimensionIndexChanged, this, &DimensionPlot::onDimensionChanged);
}

void DimensionPlot::onDatasetChanged()
{
    if (!_featureDataset.isValid() || !_clusterDataset.isValid())
    {
        qWarning() << "No valid cluster dataset";
        return;
    }

    _settingsAction.getDimensionPicker()->setPointsDataset(_featureDataset);

    onDimensionChanged();
}

void DimensionPlot::onDimensionChanged()
{
    if (!_featureDataset.isValid() || !_clusterDataset.isValid())
    {
        qWarning() << "No valid cluster";
        return;
    }

    // Get dimension from picker action
    int dimensionIndex = _settingsAction.getDimensionPicker()->getCurrentDimensionIndex();

    if (dimensionIndex >= 0 && dimensionIndex < _featureDataset->getNumDimensions())
        _webWidget->setData(_featureDataset, dimensionIndex, _clusterDataset);
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
