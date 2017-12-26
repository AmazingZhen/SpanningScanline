#pragma once

#include <QtWidgets/QMainWindow>
#include <QImage>
#include <QtWidgets>

#include "ui_ModelDisplayer.h"

#include "Loader/ModelLoader.h"
#include "Render/ModelRender.h"

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;

namespace SpanningScanline {
	enum InteractionMode {
		Rotate,
		Move
	};

	class ModelDisplayer : public QMainWindow
	{
		Q_OBJECT

	public:
		ModelDisplayer(QWidget *parent = Q_NULLPTR);

	private slots:
		void open();
		void about();

	private:
		void createActions();
		void updateActions();

		void setImage(const QImage &newImage);

		// Interaction
		void keyPressEvent(QKeyEvent *event);
		void wheelEvent(QWheelEvent *event);
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);

		void updateCamera();
		void updateDisplay();

		void resetCamera();

		QImage image;
		QLabel *imageLabel;
		QScrollArea *scrollArea;
		double scaleFactor;

		Ui::ModelDisplayerClass ui;
		
		// Loader
		std::string typeFilter;
		ModelLoader loader;

		// Render
		int m_width, m_height;
		ModelRender render;

		// Interaction
		float m_camera_distance;
		float m_horizontalAngle, m_verticalAngle;
		InteractionMode m_mode;
		float m_mousePressX, m_mousePressY;

		int m_frameCount;
		int m_time;
	};
}
