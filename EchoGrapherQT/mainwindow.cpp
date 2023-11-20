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
    updateTimer->start(100); // Update every 100 milliseconds, adjust as needed


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

void MainWindow::updateSpectrogram() {
    if (!spectrumBuffer.isEmpty()) {
        // Get the dimensions for your spectrogram
        int spectrogramWidth = 800; // Width of the spectrogram image
        int spectrogramHeight = 500; // Height of the spectrogram image

        static QImage spectrogramImage(spectrogramWidth, spectrogramHeight, QImage::Format_RGB32);
        QPainter painter(&spectrogramImage);

        // Shift the spectrogram image left by one column width to make space for new data
        QImage temp = spectrogramImage.copy(1, 0, spectrogramWidth - 1, spectrogramHeight);
        spectrogramImage.fill(Qt::black); // Fill the image with black
        painter.drawImage(0, 0, temp); // Draw the shifted image onto the original image

        // The x position for the new data (rightmost column)
        int xPosition = spectrogramImage.width() - 1;

        // Draw each new column from the buffer onto the spectrogram image
        for (const QVector<float> &spectrumChunk : spectrumBuffer) {
            // Assume spectrumChunk values are normalized between 0 and 1
            for (int y = 0; y < spectrumChunk.size(); ++y) {
                // Normalize the value to be within the range [0, 1]
                float normalizedValue = std::max(0.0f, std::min(spectrumChunk[y], 1.0f));
                // Scale the value to the height of the image
                int valueHeight = static_cast<int>(normalizedValue * spectrogramHeight);
                // Calculate the hue, ensuring it is within the range [0, 1]
                float hue = 0.7f * (1.0f - normalizedValue);
                // Determine the color based on the value
                QColor color = QColor::fromHsvF(hue, 1.0f, normalizedValue);
                // Draw a vertical line representing the spectrum value
                painter.setPen(color);
                painter.drawLine(xPosition, spectrogramHeight - valueHeight, xPosition, spectrogramHeight);
            }
        }

        // Finish painting
        painter.end();

        // Now update the UI with the image
        QGraphicsScene *scene = ui->graphicsView->scene();
        if (!scene) {
            scene = new QGraphicsScene(this);
            ui->graphicsView->setScene(scene);
        } else {
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


void MainWindow::on_overlapSlider_valueChanged(int value)
{
    audioProcessor->windowOverlap = value/10.0f;
    ui->overlapLabel->setText("Overlap: " + QString::number(value * 10) + "%");
}

