#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "audioprocessor.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Slot for handling the creation of the spectrogram visualization
    void onNewSpectrogram(const QVector<float> &spectrum);
     void updateSpectrogram();
    // Slot to handle errors from the audio processor
    void onErrorOccurred(const QString &errorMessage);
    // Slots to handle button clicks for starting and stopping processing
    void startProcessing();
    void stopProcessing();
    void on_startButton_clicked();
    void on_stopButton_clicked();

    void on_selectOutputPathButton_clicked();

    void on_windowSizeslider_valueChanged(int value);

    void on_melBandSlider_valueChanged(int value);

    void on_overlapSlider_valueChanged(int value);

private:
    Ui::MainWindow *ui;
    AudioProcessor *audioProcessor; // Pointer to the AudioProcessor class
    QVector<QVector<float>> spectrumBuffer;
    QTimer *updateTimer;
};

#endif // MAINWINDOW_H
