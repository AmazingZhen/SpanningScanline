#pragma once

#include <QtWidgets/QMainWindow>
#include <QImage>

#include "ui_ModelDisplayer.h"

#include "Loader/ModelLoader.h"
#include "Render/ModelRender.h"

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;

namespace SpanningScanline {
	class ModelDisplayer : public QMainWindow
	{
		Q_OBJECT

	public:
		ModelDisplayer(QWidget *parent = Q_NULLPTR);

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

		Ui::ModelDisplayerClass ui;

		// Loader
		std::string typeFilter;
		ModelLoader loader;

		// Render
		ModelRender render;
	};
}
