#include "SettingsAction.h"

#include "DimensionPlotView.h"

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _plugin(dynamic_cast<DimensionPlotView*>(parent)),
    _dimensionAction(new DimensionPickerAction(this, "Dimension"))
{

}
