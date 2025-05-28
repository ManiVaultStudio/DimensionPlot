#pragma once

#include <actions/GroupAction.h>
#include <actions/TriggerAction.h>
#include <PointData/DimensionPickerAction.h>

class DimensionPlotView;

class SettingsAction : public mv::gui::GroupAction
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

    //mv::gui::TriggerAction& getLineRendererButton() { return _lineRendererButton; }
    //mv::gui::TriggerAction& getRealRendererButton() { return _realRendererButton; }

private:
    DimensionPlotView*              _plugin;

    DimensionPickerAction*          _dimensionAction;
    //mv::gui::TriggerAction  _lineRendererButton;
    //mv::gui::TriggerAction  _realRendererButton;
};
