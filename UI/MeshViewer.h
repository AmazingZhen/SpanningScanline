#pragma once

#include <QtWidgets/QMainWindow>
#include <QImage>

#include "ui_MeshViewer.h"

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;

class MeshViewer : public QMainWindow
{
	Q_OBJECT

public:
	MeshViewer(QWidget *parent = Q_NULLPTR);

	private slots:
	void open();
	void openImage();

private:
	void createActions();
	void updateActions();

	void setImage(const QImage &newImage);

	bool loadImageFile(const QString & fileName);

	QImage image;
	QLabel *imageLabel;
	QScrollArea *scrollArea;
	double scaleFactor;

	Ui::MeshViewerClass ui;
};
