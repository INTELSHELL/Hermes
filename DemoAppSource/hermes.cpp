#include <QtWidgets>
#include <qlabel.h>
#include <qlayout.h>
#include <qwt_wheel.h>
#include <qwt_thermo.h>
#include <qwt_scale_engine.h>
#include <qwt_transform.h>
#include <qwt_color_map.h>
#include <QColor>
#include <QGridLayout>
#include "hermes.h"
#include "configdialog.h"
#include "kit_model.h"
#include "kit_controller.h"

Hermes::Hermes(KitModel *model, KitController *controller, QWidget *parent)
	: QWizard(parent)
{
	this->controller = controller;
	this->model = model;
	tempDemoPage = new TempDemoPage(model, controller);
	moistDemoPage = new MoistureDemoPage(model, controller);
	remoteOperationPage = new RemoteOperationPage(model, controller);
	setPage(Page_Main, new MainPage(model, controller));
	setPage(Page_TempDemo, tempDemoPage);
	setPage(Page_MoistureDemo, moistDemoPage);
	setPage(Page_RemoteOperation, remoteOperationPage);
	setStartId(Page_Main);
	setWizardStyle(ModernStyle);
	setButtonText(QWizard::CancelButton, tr("&Quit"));
	disconnect(button(QWizard::CancelButton), SIGNAL(clicked()), this, SLOT(reject()));
	connect(button(QWizard::CancelButton), SIGNAL(clicked()), this, SLOT(quitButtonClicked()));	    QList<QWizard::WizardButton> layout;
	layout << QWizard::Stretch << QWizard::CancelButton;
	setButtonLayout(layout);
	setFixedSize(800, 430);
	setWindowTitle(tr("Hermes IoT Development Platform v1.1.0"));
}
void Hermes::quitButtonClicked()
{
	QFile runningFile("running");
	if (runningFile.exists())
		runningFile.remove();
	controller->turnReaderOff();
	reject();
}
MainPage::MainPage(KitModel *model, KitController *controller, QWidget *parent) : QWizardPage(parent)
{
	setTitle("Hermes IoT Development Platform");
	QFont font;
	setPixmap(QWizard::WatermarkPixmap, QPixmap(MAIN_PAGE_ICON));
	tempDemoButton = new QPushButton(tr("Temperature Demo"));
	moistureDemoButton = new QPushButton(tr("Moisture Demo"));
	remoteOperationButton = new QPushButton(tr("Remote Operation"));
	tempDemoButton->setMaximumWidth(225);
	moistureDemoButton->setMaximumWidth(225);
	remoteOperationButton->setMaximumWidth(225);
	connect(tempDemoButton, SIGNAL(clicked()),
			this, SLOT(tempDemoButtonClicked()));
	connect(moistureDemoButton, SIGNAL(clicked()),
			this, SLOT(moistureDemoButtonClicked()));
	connect(remoteOperationButton, SIGNAL(clicked()),
			this, SLOT(remoteOperationButtonClicked()));
	this->model=model;
	QVBoxLayout *optionsLayout = new QVBoxLayout;
	optionsLayout->addWidget(tempDemoButton);
	optionsLayout->addWidget(moistureDemoButton);
	optionsLayout->addWidget(remoteOperationButton);
	optionsLayout->addStretch();
	setLayout(optionsLayout);
}
void MainPage::tempDemoButtonClicked()
{
	buttonClicked = "Temp Demo";
	wizard()->next();
}
void MainPage::moistureDemoButtonClicked()
{
	buttonClicked = "Moisture Demo";
	wizard()->next();
}
void MainPage::remoteOperationButtonClicked()
{
	buttonClicked = "Remote Operation";
	wizard()->next();
}
int MainPage::nextId() const
{
	if(buttonClicked == "Temp Demo")
		return Hermes::Page_TempDemo;
	else if(buttonClicked == "Moisture Demo")
		return Hermes::Page_MoistureDemo;
	else if(buttonClicked == "Remote Operation")
		return Hermes::Page_RemoteOperation;
	else
		return Hermes::Page_Main;
}
TempDemoPage::TempDemoPage(KitModel *model, KitController *controller, QWidget *parent)
	: QWizardPage(parent)
{
	this->model = model;
	this->controller = controller;
	int maxNumberOfTags=5;	
	plot = new Chart("Temperature", model, controller);
	thread = new ChartThread;
	thread->initialize(controller, model);
	qRegisterMetaType< QList<SensorTag> >( "QList<SensorTag>" );
	qRegisterMetaType< QList<QDateTime> >( "QList<QDateTime>" );
	connect(thread, SIGNAL(tempTagsFoundSignal(QList<SensorTag>)), plot, SLOT(setTempCurvesSlot(QList<SensorTag>)));
	connect(thread, SIGNAL(tempTagsMeasuredSignal(QList<SensorTag>, QList<QDateTime>)), plot, SLOT(updateTempCurvesSlot(QList<SensorTag>, QList<QDateTime>)));
	connect(thread, SIGNAL(tempTagsMeasuredSignal(QList<SensorTag>, QList<QDateTime>)), this, SLOT(updateTempOutputLabelsSlot(QList<SensorTag>)));
	connect(model, SIGNAL(updateTempTagSelectionsSignal()), this, SLOT(updateTagSelectionsSlot()));
	connect(model, SIGNAL(updateTempTagsSignal(QList<SensorTag>)), this, SLOT(updateTempOutputLabelsSlot(QList<SensorTag>)));
	connect(model, SIGNAL(updateTempTagsSignal(QList<SensorTag>)), plot, SLOT(setTempCurvesSlot(QList<SensorTag>)));
	const int margin = 5;
	plot->setContentsMargins( margin, margin, margin, margin );
	QHBoxLayout *outputLayout = new QHBoxLayout;
	QString labelColors[] = { "color: rgb(0,0,255); background-color: rgb(232,232,232)", "color: rgb(255,0,0); background-color: rgb(232,232,232)", "color: rgb(0,200,0); background-color: rgb(232,232,232)", "color: rgb(50,200,200); background-color: rgb(232,232,232)", "color: rgb(250,150,50); background-color: rgb(232,232,232)"};
	QString info = ""; 
	for (int i=0; i<maxNumberOfTags; i++)
	{
		outputLabels.append(new QLabel(info, this));
		outputLabels[outputLabels.length()-1]->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		outputLabels[outputLabels.length()-1]->setMaximumWidth(70);
		outputLabels[outputLabels.length()-1]->setAlignment(Qt::AlignCenter);
		outputLabels[outputLabels.length()-1]->setStyleSheet(labelColors[i]); 
		outputLayout->addWidget(outputLabels[outputLabels.length()-1]);
	}
	plotLayout = new QVBoxLayout;
	plotLayout->addWidget(plot);
	plotLayout->addLayout(outputLayout);
	QWidget *plotWidget = new QWidget;
	plotWidget->setLayout(plotLayout);
	measurementDetailsButton = new QPushButton(tr("Details"));
	connect(measurementDetailsButton, SIGNAL(clicked()),
			this, SLOT(measurementDetailsButtonClicked()));
	mainScreenButton = new QPushButton(tr("Main"));
	connect(mainScreenButton, SIGNAL(clicked()),
			this, SLOT(mainScreenButtonClicked()));
	helpButton = new QPushButton(tr("Help"));
	connect(helpButton, SIGNAL(clicked()),
			this, SLOT(helpButtonClicked()));
	configButton = new QPushButton(tr("Settings"));
	connect(configButton, SIGNAL(clicked()),
			this, SLOT(configButtonClicked()));
	calibrationButton = new QPushButton(tr("Calibrate"));
	connect(calibrationButton, SIGNAL(clicked()),
			this, SLOT(calibrationButtonClicked()));
	periodLabel = new QLabel(tr("Period"));
	periodCombo = new QComboBox;
	periodCombo->addItem(tr("Fast"));
	periodCombo->addItem(tr("10 sec"));
	periodCombo->addItem(tr("20 sec"));
	periodCombo->addItem(tr("1 min"));
	periodCombo->addItem(tr("2 min"));
	startButton = new QPushButton(tr("Start")); 
	stopButton = new QPushButton(tr("Stop"));
	clearButton = new QPushButton(tr("Clear"));
	exportButton = new QPushButton(tr("Export"));
	connect(periodCombo, SIGNAL(currentIndexChanged(QString)),
			this, SLOT(periodChanged()));
	connect(startButton, SIGNAL(clicked()),
			this, SLOT(startButtonClicked()));
	connect(stopButton, SIGNAL(clicked()),
			this, SLOT(stopButtonClicked()));
	connect(clearButton, SIGNAL(clicked()),
			this, SLOT(clearButtonClicked()));
	connect(exportButton, SIGNAL(clicked()),
			this, SLOT(exportButtonClicked()));
	calibrationDialog = new CalibrationDialog(model, controller, "Temperature");
	calibrationDialog->setWindowIcon(QIcon(APPLICATION_ICON));
	configTempDemoDialog = new ConfigTempDemoDialog(model, controller);
	configTempDemoDialog->setWindowIcon(QIcon(APPLICATION_ICON));
	measurementDetailsDialog = new MeasurementDetailsDialog(model, controller, "Temperature");
	measurementDetailsDialog->setWindowIcon(QIcon(APPLICATION_ICON));
	connect(thread, SIGNAL(tempTagsMeasuredSignal(QList<SensorTag>, QList<QDateTime>)), measurementDetailsDialog->tablePage, SLOT(loadTempTableSlot(QList<SensorTag>)));
	helpDialog = new HelpDialog(model, controller, "TempDemoPage");
	helpDialog->setWindowIcon(QIcon(APPLICATION_ICON));
	QVBoxLayout *chartControlLayout = new QVBoxLayout;
	chartControlLayout->addWidget(periodLabel);
	chartControlLayout->addWidget(periodCombo);
	chartControlLayout->addWidget(startButton);
	chartControlLayout->addWidget(stopButton);
	chartControlLayout->addWidget(clearButton);
	chartControlLayout->addWidget(exportButton);
	chartControlLayout->addStretch();
	QHBoxLayout *chartAndControlLayout = new QHBoxLayout;
	chartAndControlLayout->addLayout(chartControlLayout);
	chartAndControlLayout->addWidget(plotWidget);
	QFrame *chartAndControlFrame = new QFrame;
	chartAndControlFrame->setLayout(chartAndControlLayout);
	chartAndControlFrame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	chartAndControlFrame->setLineWidth(3);
	chartAndControlFrame->setMidLineWidth(3);
	logoLabel=new QLabel();
	logoLabel->setFixedWidth(100);
	logoLabel->setFixedHeight(50);
	movie=new QMovie(ANIMATED_ICON);
	movie->setScaledSize(QSize(100,50));
	movie->jumpToFrame(0);
	logoLabel->setMovie(movie);
	setStyleSheet("QComboBox {selection-background-color:blue}");
	QGridLayout *plotFrameLayout = new QGridLayout;
	plotFrameLayout->addWidget(chartAndControlFrame, 0, 0, -1, -1);
	QVBoxLayout *demoControlLayout = new QVBoxLayout;
	demoControlLayout->addWidget(measurementDetailsButton);
	demoControlLayout->addWidget(configButton);
	demoControlLayout->addWidget(calibrationButton);
	demoControlLayout->addWidget(helpButton);
	demoControlLayout->addWidget(mainScreenButton);
	demoControlLayout->addWidget(logoLabel);
	demoControlLayout->addStretch();
	QHBoxLayout *mainLayout = new QHBoxLayout;
	mainLayout->addLayout(plotFrameLayout);
	mainLayout->addLayout(demoControlLayout);
	setLayout(mainLayout);
	stopButton->setEnabled(false);
}
void TempDemoPage::updateTagSelectionsSlot()
{
	qDebug("updateTagSelectionsSlot");
	for (int t=0; t < model->TempTagList.length(); t++)
	{
		if (model->TempTagList[t].SelectedForMeasurement==false)
		{
			outputLabels[t]->setText("");
		}
	}
}
void TempDemoPage::updateTempOutputLabelsSlot(QList<SensorTag> TempTagList)
{
	for (int i=0; i<outputLabels.length(); i++)
	{
		outputLabels[i]->setText("");
	}
	for (int t=0; t < TempTagList.length(); t++)
	{
		int historyLength = TempTagList[t].TemperatureMeasurementHistory.length();
		if (TempTagList[t].SelectedForMeasurement==true && historyLength>0)
		{		
			float temp = TempTagList[t].TemperatureMeasurementHistory[historyLength-1].getValue();
			if (temp > -1000)	
				outputLabels[t]->setText(QString::number(temp, 'f', 1)+" C");
			else
				outputLabels[t]->setText("-----");
		}
	}
}
void TempDemoPage::measurementDetailsButtonClicked()
{
	measurementDetailsDialog->exec();
}
void TempDemoPage::mainScreenButtonClicked()
{
	wizard()->back();
}
void TempDemoPage::helpButtonClicked()
{
	helpDialog->exec();
}
void TempDemoPage::configButtonClicked()
{
	configTempDemoDialog->exec();
}
void TempDemoPage::calibrationButtonClicked()
{
	int n=model->numberOfSelectedTempTags();
	if (n!=1)
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle("Error");
		msgBox.setText("Exactly 1 temperature tag must be selected for calibration");
		msgBox.setWindowIcon(QIcon(APPLICATION_ICON));
		msgBox.exec();
	}
	else
	{
		thread->abort=false;
		calibrationDialog->exec();
	}
}
void TempDemoPage::periodChanged()
{
	QString period = periodCombo->currentText();
}
void TempDemoPage::startButtonClicked()
{
	if (model->moistAutoPower==true)
		qDebug("Autopower ON");
	int period;
	QString periodString = periodCombo->currentText();
	if (periodString == "Fast")
		period=0;
	else if (periodString == "10 sec")
		period=10;
	else if (periodString == "20 sec")
		period=20;
	else if (periodString == "1 min")
		period=60;
	else if (periodString == "2 min")
		period=120;
	else
		period=0;
	configButton->setEnabled(false);
	calibrationButton->setEnabled(false);
	mainScreenButton->setEnabled(false);
	startButton->setEnabled(false);
	clearButton->setEnabled(false);
	exportButton->setEnabled(false);
	periodCombo->setEnabled(false);
	stopButton->setEnabled(true);
	thread->startCollection(TEMPERATURE, period);
	movie->start();
}
void TempDemoPage::stopButtonClicked()
{
	thread->stopCollection(TEMPERATURE);
	configButton->setEnabled(true);
	calibrationButton->setEnabled(true);
	mainScreenButton->setEnabled(true);
	startButton->setEnabled(true);
	clearButton->setEnabled(true);
	exportButton->setEnabled(true);
	periodCombo->setEnabled(true);
	stopButton->setEnabled(false);
	movie->stop();
	movie->jumpToFrame(0);
}
void TempDemoPage::clearButtonClicked()
{
	controller->clearTempTags();
}
void TempDemoPage::exportButtonClicked()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export Log File"));
	if (fileName=="")
		return;
	qDebug("file name: %s", qPrintable(fileName));
	QFile *logFile = new QFile(fileName);
	if (model->exportTempLog(logFile)!=0)
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle("Export Log");
		msgBox.setText("Could open file for writing");
		msgBox.exec();
	}
	delete logFile;
}
MoistureDemoPage::MoistureDemoPage(KitModel *model, KitController *controller, QWidget *parent)
	: QWizardPage(parent)
{
	this->model = model;
	this->controller = controller;
	int maxNumberOfTags=5;
	plot = new Chart("Moisture", model, controller);
	thread = new ChartThread;
	thread->initialize(controller, model);
	qRegisterMetaType< QList<SensorTag> >( "QList<SensorTag>" );
	qRegisterMetaType< QList<QDateTime> >( "QList<QDateTime>" );
	connect(thread, SIGNAL(moistTagsFoundSignal(QList<SensorTag>)), plot, SLOT(setMoistCurvesSlot(QList<SensorTag>)));
	connect(thread, SIGNAL(moistTagsMeasuredSignal(QList<SensorTag>, QList<QDateTime>)), plot, SLOT(updateMoistCurvesSlot(QList<SensorTag>, QList<QDateTime>)));
	connect(thread, SIGNAL(moistTagsMeasuredSignal(QList<SensorTag>, QList<QDateTime>)), this, SLOT(updateMoistOutputLabelsSlot(QList<SensorTag>)));
	connect(model, SIGNAL(updateMoistTagSelectionsSignal()), this, SLOT(updateTagSelectionsSlot()));
	connect(model, SIGNAL(updateMoistTagsSignal(QList<SensorTag>)), this, SLOT(updateMoistOutputLabelsSlot(QList<SensorTag>)));
	connect(model, SIGNAL(updateMoistTagsSignal(QList<SensorTag>)), plot, SLOT(setMoistCurvesSlot(QList<SensorTag>)));
	const int margin = 5;
	plot->setContentsMargins( margin, margin, margin, margin );
	QHBoxLayout *outputLayout = new QHBoxLayout;
	QString labelColors[] = { "color: rgb(0,0,255); background-color: rgb(232,232,232)", "color: rgb(255,0,0); background-color: rgb(232,232,232)", "color: rgb(0,200,0); background-color: rgb(232,232,232)", "color: rgb(50,200,200); background-color: rgb(232,232,232)", "color: rgb(250,150,50); background-color: rgb(232,232,232)"};
	QString info = ""; 
	for (int i=0; i<maxNumberOfTags; i++)
	{
		outputLabels.append(new QLabel(info, this));
		outputLabels[outputLabels.length()-1]->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		outputLabels[outputLabels.length()-1]->setMaximumWidth(70);
		outputLabels[outputLabels.length()-1]->setAlignment(Qt::AlignCenter);
		outputLabels[outputLabels.length()-1]->setStyleSheet(labelColors[i]);
		outputLayout->addWidget(outputLabels[outputLabels.length()-1]);
	}
	QVBoxLayout *plotLayout = new QVBoxLayout;
	plotLayout->addWidget(plot);
	plotLayout->addLayout(outputLayout);
	QWidget *plotWidget = new QWidget;
	plotWidget->setLayout(plotLayout);
	measurementDetailsButton = new QPushButton(tr("Details"));
	connect(measurementDetailsButton, SIGNAL(clicked()),
			this, SLOT(measurementDetailsButtonClicked()));
	mainScreenButton = new QPushButton(tr("Main"));
	connect(mainScreenButton, SIGNAL(clicked()),
			this, SLOT(mainScreenButtonClicked()));
	helpButton = new QPushButton(tr("Help"));
	connect(helpButton, SIGNAL(clicked()),
			this, SLOT(helpButtonClicked()));
	configButton = new QPushButton(tr("Settings"));
	connect(configButton, SIGNAL(clicked()),
			this, SLOT(configButtonClicked()));
	periodLabel = new QLabel(tr("Period"));
	periodCombo = new QComboBox;
	periodCombo->addItem(tr("Fast"));
	periodCombo->addItem(tr("10 sec"));
	periodCombo->addItem(tr("20 sec"));
	periodCombo->addItem(tr("1 min"));
	periodCombo->addItem(tr("2 min"));
	startButton = new QPushButton(tr("Start"));
	stopButton = new QPushButton(tr("Stop"));
	clearButton = new QPushButton(tr("Clear"));
	exportButton = new QPushButton(tr("Export"));
	connect(periodCombo, SIGNAL(currentIndexChanged(QString)),
			this, SLOT(periodChanged()));
	connect(startButton, SIGNAL(clicked()),
			this, SLOT(startButtonClicked()));
	connect(stopButton, SIGNAL(clicked()),
			this, SLOT(stopButtonClicked()));
	connect(clearButton, SIGNAL(clicked()),
			this, SLOT(clearButtonClicked()));
	connect(exportButton, SIGNAL(clicked()),
			this, SLOT(exportButtonClicked()));
	configMoistureDemoDialog = new ConfigMoistureDemoDialog(model, controller);
	configMoistureDemoDialog->setWindowIcon(QIcon(APPLICATION_ICON));
	measurementDetailsDialog = new MeasurementDetailsDialog(model, controller, "Moisture");
	measurementDetailsDialog->setWindowIcon(QIcon(APPLICATION_ICON));
	connect(thread, SIGNAL(moistTagsMeasuredSignal(QList<SensorTag>, QList<QDateTime>)), measurementDetailsDialog->tablePage, SLOT(loadMoistTableSlot(QList<SensorTag>)));
	helpDialog = new HelpDialog(model, controller, "MoistureDemoPage");
	helpDialog->setWindowIcon(QIcon(APPLICATION_ICON));
	QVBoxLayout *chartControlLayout = new QVBoxLayout;
	chartControlLayout->addWidget(periodLabel);
	chartControlLayout->addWidget(periodCombo);
	chartControlLayout->addWidget(startButton);
	chartControlLayout->addWidget(stopButton);
	chartControlLayout->addWidget(clearButton);
	chartControlLayout->addWidget(exportButton);
	chartControlLayout->addStretch();
	QHBoxLayout *chartAndControlLayout = new QHBoxLayout;
	chartAndControlLayout->addLayout(chartControlLayout);
	chartAndControlLayout->addWidget(plotWidget);
	QFrame *chartAndControlFrame = new QFrame;
	chartAndControlFrame->setLayout(chartAndControlLayout);
	chartAndControlFrame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	chartAndControlFrame->setLineWidth(3);
	chartAndControlFrame->setMidLineWidth(3);
	logoLabel=new QLabel();
	logoLabel->setFixedWidth(100);
	logoLabel->setFixedHeight(50);
	movie=new QMovie(ANIMATED_ICON);
	movie->setScaledSize(QSize(100,50));
	logoLabel->setMovie(movie);
	movie->jumpToFrame(0);
	setStyleSheet("QComboBox {selection-background-color:blue}");
	QGridLayout *plotFrameLayout = new QGridLayout;
	plotFrameLayout->addWidget(chartAndControlFrame, 0, 0, -1, -1);
	QVBoxLayout *demoControlLayout = new QVBoxLayout;
	demoControlLayout->addWidget(measurementDetailsButton);
	demoControlLayout->addWidget(configButton);
	demoControlLayout->addWidget(helpButton);
	demoControlLayout->addWidget(mainScreenButton);
	demoControlLayout->addWidget(logoLabel);
	demoControlLayout->addStretch();
	QHBoxLayout *mainLayout = new QHBoxLayout;
	mainLayout->addLayout(plotFrameLayout);
	mainLayout->addLayout(demoControlLayout);
	setLayout(mainLayout);
	stopButton->setEnabled(false);
}
void MoistureDemoPage::updateTagSelectionsSlot()
{
	qDebug("updateTagSelectionsSlot");
	for (int t=0; t < model->MoistTagList.length(); t++)
	{
		if (model->MoistTagList[t].SelectedForMeasurement==false)
		{
			outputLabels[t]->setText("");
		}
	}
}
void MoistureDemoPage::updateMoistOutputLabelsSlot(QList<SensorTag> MoistTagList)
{
	for (int i=0; i<outputLabels.length(); i++)
	{
		outputLabels[i]->setText("");
	}
	for (int t=0; t < MoistTagList.length(); t++)
	{
		int historyLength = MoistTagList[t].SensorMeasurementHistory.length();
		if (MoistTagList[t].SelectedForMeasurement==true && historyLength>0)
		{		
			float moist = MoistTagList[t].SensorMeasurementHistory[historyLength-1].getValue();
			if (moist> -1)	
			{	
				qDebug("Updating output labels: Tag %d %s %f", t, qPrintable(MoistTagList[t].getEpc()), moist);
				QString wetdry=" DRY";
				if (model->wetAbove==true && moist>=model->moistThreshold)
					wetdry=" WET";
				if (model->wetAbove==false && moist<=model->moistThreshold)
					wetdry=" WET";
				if (moist<100)
					outputLabels[t]->setText(QString::number(moist, 'f', 1)+wetdry);
				else
					outputLabels[t]->setText(QString::number(moist, 'f', 0)+wetdry);
			}
			else
				outputLabels[t]->setText("-----");
		}
	}
}
void MoistureDemoPage::measurementDetailsButtonClicked()
{
	measurementDetailsDialog->exec();
}
void MoistureDemoPage::mainScreenButtonClicked()
{
	wizard()->back();
}
void MoistureDemoPage::helpButtonClicked()
{
	helpDialog->exec();
}
void MoistureDemoPage::configButtonClicked()
{
	configMoistureDemoDialog->exec();
}
void MoistureDemoPage::periodChanged()
{
	QString period = periodCombo->currentText();
}
void MoistureDemoPage::startButtonClicked()
{
	int period;
	QString periodString = periodCombo->currentText();
	if (periodString == "Fast")
		period=0;
	else if (periodString == "10 sec")
		period=10;
	else if (periodString == "20 sec")
		period=20;
	else if (periodString == "1 min")
		period=60;
	else if (periodString == "2 min")
		period=120;
	else
		period=0;
	configButton->setEnabled(false);
	mainScreenButton->setEnabled(false);
	startButton->setEnabled(false);
	clearButton->setEnabled(false);
	exportButton->setEnabled(false);
	periodCombo->setEnabled(false);
	stopButton->setEnabled(true);
	movie->start();
	thread->startCollection(MOISTURE, period);
}
void MoistureDemoPage::stopButtonClicked()
{
	thread->stopCollection(MOISTURE);
	configButton->setEnabled(true);
	mainScreenButton->setEnabled(true);
	startButton->setEnabled(true);
	clearButton->setEnabled(true);
	exportButton->setEnabled(true);
	periodCombo->setEnabled(true);
	stopButton->setEnabled(false);
	movie->stop();
	movie->jumpToFrame(0);
}
void MoistureDemoPage::clearButtonClicked()
{
	controller->clearMoistTags();
}
void MoistureDemoPage::exportButtonClicked()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export Log File"));
	if (fileName=="")
		return;
	qDebug("file name: %s", qPrintable(fileName));
	QFile *logFile = new QFile(fileName);
	if (model->exportMoistLog(logFile)!=0)
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle("Export Log");
		msgBox.setText("Could open file for writing");
		msgBox.exec();
	}
	delete logFile;
}
RemoteOperationPage::RemoteOperationPage(KitModel *model, KitController *controller, QWidget *parent)
	: QWizardPage(parent)
{
	this->model = model;
	this->controller = controller;
	connect(controller->getRUIThreadPointer(), SIGNAL(outputToConsole(QString, QString)), this, SLOT(outputToConsole(QString, QString)));
	console = new QTextEdit;
	console->setReadOnly(true);
	QHBoxLayout *consoleLayout = new QHBoxLayout;
	consoleLayout->addWidget(console);
	startButton = new QPushButton(tr("Start"));
	stopButton = new QPushButton(tr("Stop"));
	helpButton = new QPushButton(tr("Help"));
	mainButton = new QPushButton(tr("Main"));
	interfaceTypeLabel = new QLabel(tr("Interface:"));
	interfaceTypeCombo = new QComboBox;
	interfaceTypeCombo->addItem(tr("UART"));
	interfaceTypeCombo->addItem(tr("TCP"));
	interfaceTypeCombo->addItem(tr("CAN"));
	interfaceTypeCombo->addItem(tr("I2C"));
	interfaceTypeCombo->addItem(tr("SPI"));
	interfaceTypeCombo->addItem(tr("ZIGBEE"));
	connect(startButton, SIGNAL(clicked()),
			this, SLOT(startButtonClicked()));
	connect(stopButton, SIGNAL(clicked()),
			this, SLOT(stopButtonClicked()));
	connect(helpButton, SIGNAL(clicked()),
			this, SLOT(helpButtonClicked()));
	connect(mainButton, SIGNAL(clicked()),
			this, SLOT(mainButtonClicked()));
	helpDialog = new HelpDialog(model, controller, "RemoteOperationPage");
	helpDialog->setWindowIcon(QIcon(APPLICATION_ICON));
	QFrame *consoleFrame = new QFrame;
	consoleFrame->setLayout(consoleLayout);
	consoleFrame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	consoleFrame->setLineWidth(3);
	consoleFrame->setMidLineWidth(3);
	setStyleSheet("QComboBox {selection-background-color:blue}");
	QVBoxLayout *remoteOperationControlLayout = new QVBoxLayout;
	remoteOperationControlLayout->addWidget(interfaceTypeLabel);
	remoteOperationControlLayout->addWidget(interfaceTypeCombo);
	remoteOperationControlLayout->addWidget(startButton);
	remoteOperationControlLayout->addWidget(stopButton);
	remoteOperationControlLayout->addWidget(helpButton);
	remoteOperationControlLayout->addWidget(mainButton);
	remoteOperationControlLayout->addStretch();
	QGridLayout *consoleFrameLayout = new QGridLayout;
	consoleFrameLayout->addWidget(consoleFrame, 0, 0, -1, -1);
	QHBoxLayout *mainLayout = new QHBoxLayout;
	mainLayout->addLayout(remoteOperationControlLayout);
	mainLayout->addLayout(consoleFrameLayout);
	setLayout(mainLayout);
	stopButton->setEnabled(false);
}
void RemoteOperationPage::startButtonClicked()
{
	console->clear();
	QString interfaceType = interfaceTypeCombo->currentText();
	outputToConsole("Initializing interface, please wait...\n", "Red");
	startButton->setEnabled(false);
	stopButton->setEnabled(true);
	mainButton->setEnabled(false);
	if(controller->launchRUI(interfaceType) != 0)
	{
		outputToConsole("Could not start RUI!\n", "Red");
		startButton->setEnabled(true);
		stopButton->setEnabled(false);
		mainButton->setEnabled(true);
	}
}
void RemoteOperationPage::stopButtonClicked()
{
	controller->stopRUI();
	startButton->setEnabled(true);
	stopButton->setEnabled(false);
	mainButton->setEnabled(true);
}
void RemoteOperationPage::helpButtonClicked()
{
	helpDialog->exec();
}
void RemoteOperationPage::mainButtonClicked()
{
	wizard()->back();
}
void RemoteOperationPage::outputToConsole(QString text, QString color)
{
	console->moveCursor(QTextCursor::End);
	console->insertHtml(QString("<font color=\"") % color % QString("\">") % text % QString("</font><br>"));
	console->moveCursor(QTextCursor::End);
}
