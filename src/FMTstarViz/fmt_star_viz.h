#ifndef FMTSTAR_VIZ_H_
#define FMTSTAR_VIZ_H_

#include <QLabel>

#include "fmt_star.h"
#include "path_planning_info.h"

class FMTstarViz : public QLabel
{
    Q_OBJECT
public:
    explicit FMTstarViz(QWidget *parent = 0);
    void setTree(FMTstar* p_tree);
    bool drawPath(QString filename);

    PathPlanningInfo m_PPInfo;
signals:
    
public slots:

private:
    void drawPathOnMap(QPixmap& map);
    FMTstar* mp_tree;

private slots:
    void paintEvent(QPaintEvent * e);
};

#endif // FMTSTAR_VIZ_H_
