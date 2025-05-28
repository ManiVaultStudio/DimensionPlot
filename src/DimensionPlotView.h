#pragma once

#include "PlotWebWidget.h"

#include "SettingsAction.h"

#include <ViewPlugin.h>

#include <PointData/PointData.h>
#include <ClusterData/ClusterData.h>
#include <TextData/TextData.h>

#include <widgets/DropWidget.h>
#include <actions/StringAction.h>
#include <actions/HorizontalToolbarAction.h>

#include <QWidget>

/** All plugin related classes are in the MV plugin namespace */
using namespace mv::plugin;

/** Drop widget used in this plugin is located in the MV gui namespace */
using namespace mv::gui;

/** Dataset reference used in this plugin is located in the MV util namespace */
using namespace mv::util;

class QLabel;

/**
 * View plugin class
 *
 * @authors J. Thijssen
 */
class DimensionPlotView : public ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    DimensionPlotView(const PluginFactory* factory);

    /** Destructor */
    ~DimensionPlotView() override = default;
    
    /** This function is called by the core after the view plugin has been created */
    void init() override;

private:
    void onDatasetChanged();
    void onDimensionChanged();
    void onDimensionChanged2();

private:
    HorizontalToolbarAction         _primaryToolbarAction;      /** Horizontal toolbar for primary content */
    SettingsAction                  _settingsAction;
    DropWidget*                     _dropWidget;                /** Widget for drag and drop behavior */
    mv::Dataset<Points>             _currentDataSet;            // Reference to currently shown data set
    mv::Dataset<Clusters>           _clusterDataset;            // Reference to cluster dataset
    mv::Dataset<Text>               _cellMetadata;

    PlotWebWidget*                  _webWidget;
};

/**
 * View plugin factory class
 *
 * Note: Factory does not need to be altered (merely responsible for generating new plugins when requested)
 */
class DimensionPlotViewFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.DimensionPlotView"
                      FILE  "DimensionPlotView.json")

public:

    /** Default constructor */
    DimensionPlotViewFactory() {}

    /** Destructor */
    ~DimensionPlotViewFactory() override {}
    
    /** Creates an instance of the example view plugin */
    ViewPlugin* produce() override;

    /** Returns the data types that are supported by the example view plugin */
    mv::DataTypes supportedDataTypes() const override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;
};
