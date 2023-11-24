#include <portaudio.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QTimer>
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
#include <QLinearGradient>
#include <QGraphicsRectItem>

#include <cstdio>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow),
                                          dragPosition(0, 0),
                                          dragging(false),
                                          audioProcessor(new AudioProcessor(this))
{
    setWindowFlags(Qt::FramelessWindowHint); // Set frameless window

    ui->setupUi(this); // Set up the UI as defined by the .ui file

    // Create the custom title bar
    titleBar = new QWidget();
    titleBar->setStyleSheet("background-color: #1b1d27;");
    titleBar->setFixedHeight(60);

    // Load the application icon
    QLabel *iconLabel = new QLabel(titleBar);
    QPixmap appIcon(":/assets/appicon.png");
    iconLabel->setPixmap(appIcon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation)); // Scale the icon to fit the title bar
    iconLabel->setContentsMargins(0, 0, 60, 0);

    // Create the title label with centered alignment
    QLabel *titleLabel = new QLabel("EchoGrapher", titleBar);
    titleLabel->setAlignment(Qt::AlignCenter);
    //    titleLabel->setContentsMargins(60, 0, 0, 0);

    // Customize the font
    QFont titleFont = titleLabel->font();
    titleFont.setFamily("Arial"); // Set the font family to Arial or any other
    titleFont.setPointSize(12);   // Set the font size
    titleFont.setBold(true);      // Make the font bold
    titleLabel->setFont(titleFont);

    // Set the text color using a style sheet
    titleLabel->setStyleSheet("QLabel { color : white; }"); // Set the color to white or any other

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0); // Remove margins from the title layout

    titleLayout->addWidget(iconLabel);     // Add the icon label to the layout
    titleLayout->addStretch();             // Spacer to push the title to the center
    titleLayout->addWidget(titleLabel, 1); // Add the title label to the layout
    titleLayout->addStretch();             // Another spacer to ensure the title stays centered

    // Set up button size and icon size
    const QSize buttonSize(32, 32);

    QPushButton *minimizeButton = new QPushButton(titleBar);
    minimizeButton->setIcon(QIcon(":/assets/minimize.png"));
    minimizeButton->setIconSize(buttonSize);
    minimizeButton->setFixedSize(buttonSize);
    minimizeButton->setFlat(true);

    QPushButton *maximizeButton = new QPushButton(titleBar);
    maximizeButton->setIcon(QIcon(":/assets/maximize.png"));
    maximizeButton->setIconSize(buttonSize);
    maximizeButton->setFixedSize(buttonSize);
    maximizeButton->setFlat(true);

    QPushButton *closeButton = new QPushButton(titleBar);
    closeButton->setIcon(QIcon(":/assets/close.png"));
    closeButton->setIconSize(buttonSize);
    closeButton->setFixedSize(buttonSize);
    closeButton->setFlat(true);

    //    closeButton->setContentsMargins(0, 0, 60, 0);
    titleLayout->addWidget(minimizeButton);
    titleLayout->addWidget(maximizeButton);
    titleLayout->addWidget(closeButton);

    // Add a fixed-width spacer to push the buttons to the left
    int spacerWidth = 10;
    QSpacerItem *rightSpacer = new QSpacerItem(spacerWidth, 1, QSizePolicy::Fixed, QSizePolicy::Fixed);
    titleLayout->addSpacerItem(rightSpacer);

    connect(maximizeButton, &QPushButton::clicked, this, &MainWindow::toggleMaximizeRestore);
    connect(minimizeButton, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::close);

    // Add the custom title bar to the main window layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0); // Remove margins
    mainLayout->addWidget(titleBar);            // Add title bar first
    mainLayout->addWidget(ui->centralwidget);   // Then add the central widget from the UI file

    // Create a container widget for the layout
    QWidget *container = new QWidget();
    container->setLayout(mainLayout); // Set the layout on the container

    setCentralWidget(container); // Set the container as the central widget

    ui->overlapLabel->setText("Overlap: 50%");
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateSpectrogram);
    updateTimer->start(100); // Update every 100 milliseconds
    this->setWindowIcon(QIcon(":/assets/appicon.png"));
    ui->outputPathLineEdit->setText(audioProcessor->setOutputPath(""));
    this->setStyleSheet("QMainWindow { background-color: #333; } QLabel, QPushButton { color: #FFF; }");
    InitializePortAudio();

    // Set up the initial QGraphicsScene for the spectrogram visualization
    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->setBackgroundBrush(QBrush(Qt::black));

    // Connect the buttons to the slots in the MainWindow constructor
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::startProcessing);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::stopProcessing);

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

void MainWindow::InitializePortAudio()
{
    // Flush stderr to ensure all pending error messages are written
    fflush(stderr);

// Save the original stderr file descriptor
#if defined(_WIN32)
    int old_stderr = _dup(_fileno(stderr));
    freopen("nul", "w", stderr);
#else
    int old_stderr = dup(fileno(stderr));
    freopen("/dev/null", "w", stderr);
#endif

    // Initialize PortAudio (or other operations that generate unwanted messages)
    PaError err = Pa_Initialize(); // Calling Library initialization function
    if (err != paNoError)
    {
        QMessageBox::critical(this, tr("PortAudio Error"), tr("Error initializing PortAudio: %1").arg(Pa_GetErrorText(err)));
    }

    // Flush stderr again to ensure any error messages are written to nul or /dev/null
    fflush(stderr);

// Restore the original stderr file descriptor
#if defined(_WIN32)
    _dup2(old_stderr, _fileno(stderr));
    _close(old_stderr);
#else
    dup2(old_stderr, fileno(stderr));
    ::close(old_stderr);
#endif
}

void MainWindow::toggleMaximizeRestore()
{
    if (isMaximized())
    {
        showNormal();
    }
    else
    {
        showMaximized();
    }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Check if the double-click is within the title bar area
    if (titleBar->rect().contains(titleBar->mapFromGlobal(event->globalPos())))
    {
        MainWindow::toggleMaximizeRestore();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (titleBar->rect().contains(titleBar->mapFromGlobal(event->globalPos())))
        {
            dragPosition = event->globalPos() - frameGeometry().topLeft();
            dragging = true;
            event->accept();
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && dragging)
    {
        const QPoint newGlobalPos = event->globalPos();
        QPoint newPos = newGlobalPos - dragPosition;

        // Get the screen that contains the cursor
        QScreen *screen = QGuiApplication::screenAt(newGlobalPos);
        if (!screen)
        {
            return; // If for some reason no screen is found, do nothing
        }

        // Get the available geometry of the screen
        QRect availableGeometry = screen->availableGeometry();

        // Define margins that should always remain visible on screen
        const int visibleMargin = 10; // Only 10 pixels of the window header will remain visible

        // Adjust the new position to ensure the window's visible margins remain on-screen
        int newX = qMax(availableGeometry.left() - width() + visibleMargin, newPos.x());
        newX = qMin(availableGeometry.right() - visibleMargin, newX);

        int newY = qMax(availableGeometry.top() - height() + visibleMargin, newPos.y());
        newY = qMin(availableGeometry.bottom() - visibleMargin, newY);
        qDebug() << "newX:" << newX << "newY:" << newY;
        move(newX, newY);
        event->accept();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "Mouse Release Event";
    Q_UNUSED(event);
    dragging = false; // Stop dragging
}

void MainWindow::startProcessing()
{
    emit processingStarted(); // Emit the signal to notify other parts of the app
    this->setFocus(Qt::OtherFocusReason);
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
    emit processingStopped();
    this->setFocus(Qt::OtherFocusReason);
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

void MainWindow::onNewSpectrogram(const QVector<float> &spectrum)
{
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

void MainWindow::on_selectOutputPathButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    setOutputPath(dir);
}

void MainWindow::setOutputPath(const QString &path)
{
    ui->outputPathLineEdit->setText(path);
    // Pass the path to the audio processor
    audioProcessor->setOutputPath(path);
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
    float overlap = value * 0.5f; // Convert the slider value to the actual overlap percentage

    audioProcessor->windowOverlap = overlap / 100.0f;                                // Convert percentage to a fraction for the audio processor
    ui->overlapLabel->setText("Overlap: " + QString::number(overlap, 'f', 1) + "%"); // Display the value with one decimal place
}

void MainWindow::on_zoomInButton_clicked()
{
    ui->graphicsView->scale(1.1, 1.1); // Zoom in by 10%
}

void MainWindow::on_zoomOutButton_clicked()
{
    ui->graphicsView->scale(0.9, 0.9); // Zoom out by 10%
}

void MainWindow::on_resetZoomButton_clicked()
{
    ui->graphicsView->resetTransform();
}
