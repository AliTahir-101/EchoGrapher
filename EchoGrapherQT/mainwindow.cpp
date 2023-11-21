#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QGraphicsRectItem>
#include <QLinearGradient>
#include <iostream>
#include <portaudio.h>
#include <QFileDialog>
#include <QTimer>
using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow),
                                          audioProcessor(new AudioProcessor(this))
{

    ui->setupUi(this);
    ui->overlapLabel->setText("Overlap: 50%");
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateSpectrogram);
    updateTimer->start(100); // Update every 100 milliseconds

    ui->outputPathLineEdit->setText(audioProcessor->setOutputPath(""));

    PaError err = Pa_Initialize(); // Calling Library initialization function
    if (err != paNoError)
    {
        QMessageBox::critical(this, tr("PortAudio Error"), tr("Error initializing PortAudio: %1").arg(Pa_GetErrorText(err)));
    }

    // Set up the initial QGraphicsScene for the spectrogram visualization
    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->setBackgroundBrush(QBrush(Qt::black));
    this->setMaximumSize(QSize(1260, 820));
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
    ui->labelStatus->setText("Status: Processing...");

    ui->windowSizeslider->setEnabled(false);
    ui->melBandSlider->setEnabled(false);
    ui->overlapSlider->setEnabled(false);
}

void MainWindow::stopProcessing()
{
    audioProcessor->stopProcessing();  // Stop processing
    ui->startButton->setEnabled(true); // Re-enable start button
    ui->startButton->setStyleSheet("QPushButton { color: white; }");
    ui->stopButton->setEnabled(false); // Disable stop button
    ui->stopButton->setStyleSheet("QPushButton { color: gray; }");
    ui->labelStatus->setText("Status: Stopped");

    ui->windowSizeslider->setEnabled(true);
    ui->melBandSlider->setEnabled(true);
    ui->overlapSlider->setEnabled(true);
}

 void MainWindow::onNewSpectrogram(const QVector<float> &spectrum) {
     // Append the new spectrum data to the buffer instead of updating the UI directly
    spectrumBuffer.append(spectrum);
 }

void MainWindow::updateSpectrogram()
{
    if (!spectrumBuffer.isEmpty())
    {
        // Dimensions for spectrogram
        int spectrogramWidth = 800;  // Width of the spectrogram image
        int spectrogramHeight = 500; // Height of the spectrogram image

        static QImage spectrogramImage(spectrogramWidth, spectrogramHeight, QImage::Format_RGB32);
        QPainter painter(&spectrogramImage);

        // Shift the spectrogram image left by one column width to make space for new data
        QImage temp = spectrogramImage.copy(1, 0, spectrogramWidth - 1, spectrogramHeight);
        spectrogramImage.fill(Qt::black); // Fill the image with black
        painter.drawImage(0, 0, temp);    // Draw the shifted image onto the original image

        // The x position for the new data (rightmost column)
        int xPosition = spectrogramImage.width() - 1;

        // Draw each new column from the buffer onto the spectrogram image
        for (const QVector<float> &spectrum : spectrumBuffer)
        {
            for (int i = 0; i < spectrum.size(); ++i)
            {
                int barHeight = std::max(0.0f, spectrum[i]) * spectrogramHeight;
                QLinearGradient gradient(0, spectrogramHeight - barHeight, 0, spectrogramHeight);
                gradient.setColorAt(0.0, Qt::darkBlue);
                gradient.setColorAt(0.2, Qt::blue);
                gradient.setColorAt(0.4, Qt::cyan);
                gradient.setColorAt(0.6, Qt::green);
                gradient.setColorAt(0.8, Qt::yellow);
                gradient.setColorAt(1.0, Qt::red);
                painter.setBrush(gradient);
                painter.setPen(Qt::NoPen);
                // Draw the bar at the current column
                painter.drawRect(xPosition, spectrogramHeight - barHeight, 1, barHeight);
            }
        }

        // Finish painting
        painter.end();

        // Now update the UI with the image
        QGraphicsScene *scene = ui->graphicsView->scene();
        if (!scene)
        {
            scene = new QGraphicsScene(this);
            ui->graphicsView->setScene(scene);
        }
        else
        {
            scene->clear(); // Clear the previous scene
        }

        // Add the updated spectrogram image to the scene
        scene->addPixmap(QPixmap::fromImage(spectrogramImage));

        // Ensure the latest column is visible
        ui->graphicsView->ensureVisible(scene->sceneRect());

        // Clear the buffer after it has been processed
        spectrumBuffer.clear();
    }
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

void MainWindow::on_windowSizeslider_valueChanged(int value)
{
    audioProcessor->windowSize = value;
    ui->windowSlabel->setText("Window Size: " + QString::number(value));
}


void MainWindow::on_melBandSlider_valueChanged(int value)
{
    audioProcessor->numMelFilters = value;
    ui->melBandFLabel->setText("Mel Bands: " + QString::number(value));
}


//void MainWindow::on_overlapSlider_valueChanged(int value)
//{
//    audioProcessor->windowOverlap = value/10.0f;
//    ui->overlapLabel->setText("Overlap: " + QString::number(value * 10) + "%");
//}

void MainWindow::on_overlapSlider_valueChanged(int value)
{
    // Assuming 'value' ranges from 0 to 200 if we want a range from 0% to 100% with 0.5% steps
    // This means the slider actually has 201 positions (including 0)
    float overlap = value * 0.5f; // Convert the slider value to the actual overlap percentage

    audioProcessor->windowOverlap = overlap / 100.0f; // Convert percentage to a fraction for the audio processor
    ui->overlapLabel->setText("Overlap: " + QString::number(overlap, 'f', 1) + "%"); // Display the value with one decimal place
}

