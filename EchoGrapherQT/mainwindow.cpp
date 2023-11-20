#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QGraphicsRectItem>
#include <QLinearGradient>
#include <iostream>
#include <portaudio.h>
#include <QFileDialog>
using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow),
                                          audioProcessor(new AudioProcessor(this))
{

    ui->setupUi(this);
    QString dir;
    dir = audioProcessor->setOutputPath("");

    ui->outputPathLineEdit->setText(dir);

    PaError err = Pa_Initialize(); // Calling Library initialization function
    if (err != paNoError)
    {
        QMessageBox::critical(this, tr("PortAudio Error"), tr("Error initializing PortAudio: %1").arg(Pa_GetErrorText(err)));
    }

    // Set up the initial QGraphicsScene for the spectrogram visualization
    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->setBackgroundBrush(QBrush(Qt::black));
    this->setMaximumSize(QSize(800, 520));
    // Connect the AudioProcessor signals to the MainWindow slots
    connect(audioProcessor, &AudioProcessor::newLogMelSpectrogram, this, &MainWindow::onNewSpectrogram);
    connect(audioProcessor, &AudioProcessor::errorOccurred, this, &MainWindow::onErrorOccurred);
}

MainWindow::~MainWindow()
{
    MainWindow::stopProcessing();
    Pa_Terminate();
    delete ui;
}

void MainWindow::startProcessing()
{
    audioProcessor->startProcessing();  // Start with desired sample rate
    ui->startButton->setEnabled(false); // Disable start button
    ui->startButton->setStyleSheet("QPushButton { color: gray; }");
    ui->stopButton->setEnabled(true); // Enable stop button
    ui->stopButton->setStyleSheet("QPushButton { color: white; }");
    ui->label->setText("Processing...");
}

void MainWindow::stopProcessing()
{
    audioProcessor->stopProcessing();  // Stop processing
    ui->startButton->setEnabled(true); // Re-enable start button
    ui->startButton->setStyleSheet("QPushButton { color: white; }");
    ui->stopButton->setEnabled(false); // Disable stop button
    ui->stopButton->setStyleSheet("QPushButton { color: gray; }");
    ui->label->setText("Stopped");
}

void MainWindow::onNewSpectrogram(const QVector<float> &spectrum)
{
    // Scene is already created in the constructor
    QGraphicsScene *scene = ui->graphicsView->scene();
    if (!scene)
    {
        // This should not happen if the scene is set up correctly in the constructor
        scene = new QGraphicsScene(this);
        ui->graphicsView->setScene(scene);
    }

    scene->clear(); // Clear the previous items

    const int barWidth = 5;       // Width of the spectrogram bars
    const int maxBarHeight = 100; // Maximum height of the bars
    int x = 0;                    // Initial x position for the first bar

    // Iterate over the spectrum data and create bars
    for (float value : spectrum)
    {
        // Convert the log value to a positive number for visualization
        float positiveValue = value > 0 ? value : 0;
        float barHeight = positiveValue / -13.8155 * maxBarHeight; // Scale the bar height

        // Create a gradient to fill the bar
        QLinearGradient gradient(0, maxBarHeight - barHeight, 0, maxBarHeight);
        // Define color stops for the gradient from bottom to top
        gradient.setColorAt(0.0, Qt::darkBlue); // Lowest intensity
        gradient.setColorAt(0.2, Qt::blue);
        gradient.setColorAt(0.4, Qt::cyan);
        gradient.setColorAt(0.6, Qt::green);
        gradient.setColorAt(0.8, Qt::yellow);
        gradient.setColorAt(1.0, Qt::red); // Highest intensity
        // Create a rectangle item with the gradient
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(x, maxBarHeight - barHeight, barWidth, barHeight);
        rectItem->setBrush(gradient);

        // Add the rectangle to the scene
        scene->addItem(rectItem);

        // Move to the position for the next bar
        x += barWidth;
    }

    // Update the scene to reflect the new items
    scene->update();
}

void MainWindow::onErrorOccurred(const QString &errorMessage)
{
    QMessageBox::critical(this, tr("Error"), errorMessage);
    stopProcessing(); // Stop processing if there's an error
}

void MainWindow::on_startButton_clicked()
{
    MainWindow::startProcessing();
    this->setFocus(Qt::OtherFocusReason);
}

void MainWindow::on_stopButton_clicked()
{
    MainWindow::stopProcessing();
    this->setFocus(Qt::OtherFocusReason);
}

void MainWindow::on_selectOutputPathButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->outputPathLineEdit->setText(dir);
    // Pass the dir to the audio processor
    audioProcessor->setOutputPath(dir);
}