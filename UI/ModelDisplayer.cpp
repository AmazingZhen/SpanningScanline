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
	loader(false),
	m_width(600),
	m_height(600),
	render(qRgb(128, 128, 0)),
	m_camera_distance(5.f),
	m_horizontalAngle(0.f),
	m_verticalAngle(0.f),
	m_mode(Rotate)
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

	resize(QSize(m_width, m_height));

	typeFilter += "(";
	typeFilter += loader.getSupportedTypes();
	typeFilter += ");;All Files (*)";

	updateCamera();
	render.setWindowSize(m_width, m_height);
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

void ModelDisplayer::open() {
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open Model"), "",
		tr(typeFilter.c_str()));

	if (!fileName.isEmpty()) {
		loader = ModelLoader(false);
		bool loaded = loader.load(fileName, ModelLoader::PathType::AbsolutePath);

		if (loaded) {
			QVector<float> *vertices;
			QVector<float> *normals;
			QVector<unsigned int> *indices;

			loader.getBufferData(&vertices, &normals, &indices);
			render.setBufferData(*vertices, *normals, *indices);
			
			resetCamera();
			updateDisplay();
		}
	}
}

void SpanningScanline::ModelDisplayer::keyPressEvent(QKeyEvent *event)
{
	bool rotate_camera = false;
	bool specific_key_pressed = true;

	switch (event->key())
	{
	case Qt::Key_W:
		m_verticalAngle += 3;
		rotate_camera = true;
		break;
	case Qt::Key_S:
		m_verticalAngle -= 3;
		rotate_camera = true;
		break;
	case Qt::Key_A:
		m_horizontalAngle += 3;
		rotate_camera = true;
		break;
	case Qt::Key_D:
		m_horizontalAngle -= 3;
		rotate_camera = true;
		break;
	case Qt::Key_Q:
		render.switchPolygon(true);
		break;
	case Qt::Key_E:
		render.switchPolygon(false);
		break;
	case Qt::Key_Space:
		render.singlePolygon = !render.singlePolygon;
		break;
	default:
		specific_key_pressed = false;
		break;
	}

	if (rotate_camera) {
		updateCamera();
	}

	if (specific_key_pressed) {
		updateDisplay();
	}

	event->accept();
}

void SpanningScanline::ModelDisplayer::wheelEvent(QWheelEvent * event)
{
	if (event->delta() > 0) {
		m_camera_distance += 0.5f;
	}
	else {
		m_camera_distance -= 0.5f;
	}

	updateCamera();
	updateDisplay();

	event->accept();
}

void SpanningScanline::ModelDisplayer::mousePressEvent(QMouseEvent *event)
{
	m_mousePressX = event->x();
	m_mousePressY = event->y();

	if (event->buttons() == Qt::LeftButton) {
		m_mode = Rotate;
	}
	else {
		m_mode = Move;
	}

	event->accept();
}

void SpanningScanline::ModelDisplayer::mouseMoveEvent(QMouseEvent *event)
{
	float deltaX = event->x() - m_mousePressX;
	float deltaY = event->y() - m_mousePressY;

	if (m_mode == InteractionMode::Rotate) {
		m_horizontalAngle -= deltaX * .4f;
		m_verticalAngle -= deltaY * .4f;
	}

	m_mousePressX = event->x();
	m_mousePressY = event->y();

	event->accept();

	updateCamera();
	updateDisplay();
}

void SpanningScanline::ModelDisplayer::updateCamera()
{
	/*
	QQuaternion xRotation = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, m_verticalAngle);
	QQuaternion yRotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, m_horizontalAngle);
	// QQuaternion zRotation = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, m_verticalAngle);
	QQuaternion totalRotation = yRotation * xRotation;

	QMatrix4x4 rotation = QMatrix4x4(totalRotation.toRotationMatrix());
	QVector4D camera_pos(0, 0, m_camera_distance, 1);

	QVector3D camera_pos_rotated = (rotation * camera_pos).toVector3D();
	render.setCameraPos(camera_pos_rotated);
	*/

	QMatrix4x4 rot;
	rot.rotate(m_horizontalAngle, QVector3D(0.f, 1.f, 0.f));
	rot.rotate(m_verticalAngle, QVector3D(1.f, 0.f, 0.f));
	QVector4D camera_pos(0, 0, m_camera_distance, 1);

	QVector3D camera_pos_rotated = (rot * camera_pos).toVector3D();
	render.setCameraPos(camera_pos_rotated);
}

void SpanningScanline::ModelDisplayer::updateDisplay()
{
	QTime time;
	time.start();

	if (render.render()) {
		int delta_time = time.elapsed();
		printf("Using %d ms\n", delta_time);

		setImage(render.getRenderResult());
	}
}

void SpanningScanline::ModelDisplayer::resetCamera()
{
	m_camera_distance = 5.f;
	m_horizontalAngle = 0.f;
	m_verticalAngle = 0.f;

	updateCamera();
}


