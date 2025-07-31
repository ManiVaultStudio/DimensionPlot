#pragma once

#include <widgets/WebWidget.h>

#include <PointData/PointData.h>
#include <ClusterData/ClusterData.h>

#include <vector>

class DimensionPlot;

class JSCommunicationObject : public mv::gui::WebCommunicationObject
{
    Q_OBJECT
public:
    JSCommunicationObject();

signals:
    // Signals from Qt to JS side
signals:
    void setData(QString data);
    void setFilterInJS(const QVariantList& data);
    void setHeaderOptions(const QVariantList& data);

public slots:
    // Invoked from JS side 
    void js_partitionHovered(const QString& data);

signals:
    // Signals from comm object to web widget
    void partitionHovered(QString name);

private:

};

class PlotWebWidget : public mv::gui::WebWidget
{
    Q_OBJECT
public:
    PlotWebWidget(DimensionPlot* plugin);
    ~PlotWebWidget();

    JSCommunicationObject& getCommObject() { return _commObject; }

    void setData(mv::Dataset<Points>& points, int dim, const QStringList& categories, const std::vector<int>& categoryIds);
    void setData(mv::Dataset<Points>& points, int dim, mv::Dataset<Clusters>& clusterData);

private slots:
    void onWebPageFullyLoaded();
    void onPartitionHovered(QString name);

protected:
    void resizeEvent(QResizeEvent* event);

public slots:
    void applyAspectRatio();

private:
    JSCommunicationObject   _commObject;    // Communication Object between Qt (cpp) and JavaScript
};
