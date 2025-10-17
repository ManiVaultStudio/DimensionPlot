#include "SettingsAction.h"

#include "DimensionPlot.h"

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    HorizontalGroupAction(parent, title),
    _plugin(dynamic_cast<DimensionPlot*>(parent)),
    _dimensionAction(new DimensionPickerAction(this, "Dimension")),
    _metadataPicker(new DatasetPickerAction(this, "Metadata"))
{
    setLabelSizingType(LabelSizingType::Auto);

    addAction(_dimensionAction);
    addAction(_metadataPicker);
}
