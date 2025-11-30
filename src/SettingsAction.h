#pragma once

#include <actions/GroupAction.h>
#include <actions/HorizontalGroupAction.h>
#include <actions/TriggerAction.h>
#include <PointData/DimensionPickerAction.h>
#include <actions/DatasetPickerAction.h>

class DimensionPlot;

class SettingsAction : public mv::gui::HorizontalGroupAction
{
public:
    /**
     * Construct with \p parent object and \p title
     * @param parent Pointer to parent object
     * @param title Title
     */
    Q_INVOKABLE SettingsAction(QObject* parent, const QString& title);

public: // Action getters
    DatasetPickerAction& getCurrentDatasetAction() { return _currentDatasetAction; }

    DimensionPickerAction* getDimensionPicker() { return _dimensionAction; }

    DatasetPickerAction* getMetadataPicker() { return _metadataPicker; }

public: // Serialization
    /**
     * Load plugin from variant map
     * @param Variant map representation of the plugin
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save plugin to variant map
     * @return Variant map representation of the plugin
     */
    QVariantMap toVariantMap() const override;

private:
    DimensionPlot*              _plugin;

    DatasetPickerAction         _currentDatasetAction;
    DimensionPickerAction*      _dimensionAction;
    DatasetPickerAction*        _metadataPicker;
};
