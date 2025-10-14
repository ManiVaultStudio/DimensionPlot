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
    DimensionPickerAction* getDimensionPicker() { return _dimensionAction; }

    DatasetPickerAction* getMetadataPicker() { return _metadataPicker; }

private:
    DimensionPlot*              _plugin;

    DimensionPickerAction*          _dimensionAction;
    DatasetPickerAction*            _metadataPicker;
};
