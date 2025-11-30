#include "SettingsAction.h"

#include "DimensionPlot.h"

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    HorizontalGroupAction(parent, title),
    _plugin(dynamic_cast<DimensionPlot*>(parent)),
    _currentDatasetAction(this, "CurrentDataset"),
    _dimensionAction(new DimensionPickerAction(this, "Dimension")),
    _metadataPicker(new DatasetPickerAction(this, "Metadata"))
{
    setLabelSizingType(LabelSizingType::Auto);

    addAction(_dimensionAction);
    addAction(_metadataPicker);
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    _currentDatasetAction.fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _currentDatasetAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
