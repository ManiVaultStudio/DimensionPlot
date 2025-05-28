#include "PlotWebWidget.h"

#include "DimensionPlotView.h"

#include <util/Timer.h>

#include <QLayout>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QHash>

#include <iostream>
#include <random>

// =============================================================================
// JSCommunicationObject
// =============================================================================

JSCommunicationObject::JSCommunicationObject()
{

}

// =============================================================================
// PlotWebWidget
// =============================================================================

PlotWebWidget::PlotWebWidget(DimensionPlotView* plugin) :
    _commObject()
{
    connect(this, &WebWidget::webPageFullyLoaded, this, &PlotWebWidget::onWebPageFullyLoaded);

    // For more info on drag&drop behavior, see the ExampleViewPlugin project
    setAcceptDrops(true);

    // Ensure linking to the resources defined in res/ephys_viewer_resources.qrc
    Q_INIT_RESOURCE(dimplot_resources);

    // ManiVault and Qt create a "QtBridge" object on the js side which represents _comObject
    // there, we can connect the signals qt_js_* and call the slots js_qt_* from our communication object
    init(&_commObject);

    setContentsMargins(0, 0, 0, 0);
    layout()->setContentsMargins(0, 0, 0, 0);
}

PlotWebWidget::~PlotWebWidget()
{

}

void PlotWebWidget::setData(mv::Dataset<Points>& points, int dim, const QStringList& categories, const std::vector<int>& categoryIds)
{
    Timer t("SetData");
    qDebug() << "Categories: " << categories;
    std::vector<float> dimValues;
    points->extractDataForDimension(dimValues, dim);
    qDebug() << dimValues[0] << dimValues[1] << dimValues[2];
    QJsonArray vals;
    for (int i = 0; i < dimValues.size(); i++)
    {
        vals.append(dimValues[i]);
    }

    QJsonArray categoryIdArray;
    for (int i = 0; i < dimValues.size(); i++)
    {
        categoryIdArray.append(categoryIds[i]);
    }

    QJsonArray categoryArray;
    for (int i = 0; i < categories.size(); i++)
    {
        categoryArray.append(categories[i]);
    }

    QJsonObject rootObj;
    rootObj.insert("title", points->getDimensionNames()[dim]);
    rootObj.insert("values", vals);
    rootObj.insert("categories", categoryArray);
    rootObj.insert("categoryIds", categoryIdArray);

    QJsonDocument doc(rootObj);
    QString strJson(doc.toJson(QJsonDocument::Indented));

    //t.printElapsedTime("SetData", true);
    _commObject.setData(strJson);
}

void PlotWebWidget::setData(mv::Dataset<Points>& points, int dim, mv::Dataset<Clusters>& clusterData)
{
    std::vector<QString> clusterNames = clusterData->getClusterNames();

    if (clusterNames.size() != clusterData->getClusters().size())
    {
        return;
    }

    // Make categories array
    QJsonArray categories;
    QHash<QString, std::vector<uint32_t>> clusterIndices;
    for (int i = 0; i < clusterData->getClusters().size(); i++)
    {
        const Cluster& cluster = clusterData->getClusters()[i];
        const QString& clusterName = clusterNames[i];

        QJsonArray indexArray;
        for (int i = 0; i < cluster.getIndices().size(); i++)
        {
            indexArray.push_back((int)cluster.getIndices()[i]);
        }

        QJsonObject category;
        category.insert("name", clusterName);
        category.insert("indices", indexArray);
        category.insert("color", cluster.getColor().name());

        categories.append(category);
    }

    // Make values array
    std::vector<float> dimValues;
    points->extractDataForDimension(dimValues, dim);

    QJsonArray vals;
    for (int i = 0; i < dimValues.size(); i++)
    {
        vals.append(dimValues[i]);
    }

    // Make document
    QJsonObject rootObj;
    rootObj.insert("title", points->getDimensionNames()[dim]);
    rootObj.insert("values", vals);
    rootObj.insert("categories", categories);

    QJsonDocument doc(rootObj);
    QString strJson(doc.toJson(QJsonDocument::Indented));
    qDebug() << doc.toJson(QJsonDocument::Compact);

    //t.printElapsedTime("SetData", true);
    _commObject.setData(strJson);
}

void JSCommunicationObject::js_partitionHovered(const QString& data) {
    if (!data.isEmpty())
    {
        qDebug() << "PARTITION SIGNAL" << data;
        emit partitionHovered(data);
    }
}

void PlotWebWidget::onWebPageFullyLoaded()
{
    qDebug() << "EphysWebWidget::onWebPageFullyLoaded: Web page completely loaded.";
    //emit webPageLoaded();

    qDebug() << "EphysWebWidget size: " << width() << height();
}

void PlotWebWidget::onPartitionHovered(QString name)
{
    qDebug() << "You hovered over partition: " << name;
}

void PlotWebWidget::resizeEvent(QResizeEvent* event)
{
    (void)event;
    //applyAspectRatio();
}

void PlotWebWidget::applyAspectRatio()
{
    int w = this->width();
    int h = this->height();
    double aspect = static_cast<double>(h) / static_cast<double>(w);

    if (aspect < 1.0f)
    {
        int targetSize = std::max(w, h);
        setMinimumWidth(targetSize);
        setMinimumHeight(targetSize);
        setMaximumWidth(targetSize);
        setMaximumHeight(targetSize);
    }
}
