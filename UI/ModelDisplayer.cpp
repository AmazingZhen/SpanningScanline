#include "ModelDisplayer.h"
#include <iostream>
#include <QtWidgets>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QPrintDialog>
#endif
#endif

using SpanningScanline::ModelDisplayer;

ModelDisplayer::ModelDisplayer(QWidget *parent)
	: QMainWindow(parent),
	imageLabel(new QLabel),
	scrollArea(new QScrollArea),
	scaleFactor(1),
	loader(true),
	render()
{
	ui.setupUi(this);

	imageLabel->setBackgroundRole(QPalette::Base);
	imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	imageLabel->setScaledContents(true);

	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(imageLabel);
	scrollArea->setVisible(false);
	setCentralWidget(scrollArea);

	createActions();

	resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);

	typeFilter += "(";
	typeFilter += loader.getSupportedTypes();
	typeFilter += ");;All Files (*)";
}

void ModelDisplayer::createActions()
{
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

	QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &ModelDisplayer::open);
}

void ModelDisplayer::updateActions()
{
}

void ModelDisplayer::setImage(const QImage & newImage)
{
	image = newImage;
	imageLabel->setPixmap(QPixmap::fromImage(image));
	scaleFactor = 1.0;

	scrollArea->setVisible(true);
	scrollArea->setWidgetResizable(true);

	updateActions();
}

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
	static bool firstDialog = true;

	if (firstDialog) {
		firstDialog = false;
		const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
		dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
	}

	QStringList mimeTypeFilters;
	const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
		? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
	foreach(const QByteArray &mimeTypeName, supportedMimeTypes)
		mimeTypeFilters.append(mimeTypeName);
	mimeTypeFilters.sort();
	dialog.setMimeTypeFilters(mimeTypeFilters);
	dialog.selectMimeTypeFilter("image/jpeg");
	if (acceptMode == QFileDialog::AcceptSave)
		dialog.setDefaultSuffix("jpg");
}

void ModelDisplayer::open() {
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open Model"), "",
		tr(typeFilter.c_str()));

	if (!fileName.isEmpty()) {
		bool loaded = loader.load(fileName, ModelLoader::PathType::AbsolutePath);
		qDebug() << loaded;

		if (loaded) {
			QVector<float> *vertices;
			QVector<float> *normals;
			QVector<unsigned int> *indices;

			loader.getBufferData(&vertices, &normals, &indices);
			render.setBufferData(*vertices, *normals, *indices);
		}
	}
}

void ModelDisplayer::openImage()
{
	QFileDialog dialog(this, tr("Open File"));
	initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

	while (dialog.exec() == QDialog::Accepted && !loadImageFile(dialog.selectedFiles().first())) {}
}

bool ModelDisplayer::loadImageFile(const QString &fileName)
{
	QImageReader reader(fileName);
	reader.setAutoTransform(true);
	const QImage newImage = reader.read();
	if (newImage.isNull()) {
		QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
			tr("Cannot load %1: %2")
			.arg(QDir::toNativeSeparators(fileName), reader.errorString()));
		return false;
	}

	setImage(newImage);

	setWindowFilePath(fileName);

	const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
		.arg(QDir::toNativeSeparators(fileName)).arg(image.width()).arg(image.height()).arg(image.depth());
	statusBar()->showMessage(message);

	return true;
}


