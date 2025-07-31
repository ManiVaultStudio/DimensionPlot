#include "SettingsAction.h"

#include "DimensionPlot.h"

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _plugin(dynamic_cast<DimensionPlot*>(parent)),
    _dimensionAction(new DimensionPickerAction(this, "Dimension"))
{

}
