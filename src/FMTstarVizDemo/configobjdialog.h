#ifndef CONFIGOBJDIALOG_H
#define CONFIGOBJDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

class MainWindow;

class ConfigObjDialog : public QDialog {
    Q_OBJECT
public:
    ConfigObjDialog(MainWindow * parent);

    void updateConfiguration();
    void updateDisplay();

private:

    QPushButton * mpBtnOK;
    QPushButton * mpBtnCancel;

    QCheckBox * mpCheckMinDist;
    QLabel    * mpLabelMinDist;

    QLabel      * mpLabelCost;
    QLineEdit   * mpLineEditCost;
    QPushButton * mpBtnAdd;

    QLabel    * mpLabelIterationNum;
    QLineEdit * mpLineEditIterationNum;
    QLabel    * mpLabelSegmentLength;
    QLineEdit * mpLineEditSegmentLength;

    MainWindow * mpParentWindow;

    bool isCompatible(QString fitnessFile);

public slots:
    void checkBoxStateChanged(int state);
    void onBtnOKClicked();
    void onBtnCancelClicked();
    void onBtnAddClicked();

};

#endif // CONFIGOBJDIALOG_H
