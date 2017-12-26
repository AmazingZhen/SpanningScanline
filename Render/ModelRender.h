#pragma once

#include <QVector>
#include <QMatrix4x4>
#include <QImage>

#include <iostream>

using namespace std;

namespace SpanningScanline {
	struct Polygon {
		unsigned int id;

		float a, b, c;  // Normal vector of the polygon n = (a, b, c)
		double d;	// ax + by + cz + d = 0
		int cross_y;	// The number of scanlines crossed by the polygon
		
		QRgb color;
	};

	struct Side {
		unsigned int polygon_id;
		float delta_x;		// dy / dx = k, delta_x = -1 / k
		int cross_y;		// The number of scanlines crossed by the side
		float x;				// the x value of point y_max

		void print() {
			cout << "polygon_id :" << polygon_id << endl;
			cout << "delta_x :" << delta_x << endl;
			cout << "cross_y :" << cross_y << endl;
			cout << "x :" << x << endl;
		}
	};

	class ModelRender
	{
	public:
		ModelRender(QRgb backgroundColor);
		void setBufferData(const QVector<float> &vertices, const QVector<float> &normals, const QVector<unsigned int> &indices);
		bool render();
		QImage getRenderResult();

		void setCameraPos(const QVector3D &pos);
		void setModelviewMatrix(const QMatrix4x4 &m) { m_modelview = m; }
		void setWindowSize(int width, int height);

	private:
		// Initial data structure of scanline algorithm.
		bool initialPolygonTableAndSideTable();
		QVector3D getVertexFromBuffer(int index);
		QVector3D getNormalFromBuffer(int index);
		bool addPolygon(const QVector3D &a, const QVector3D &b, const QVector3D &c, float factor, int polygon_id);
		bool addSides(const QVector3D &a, const QVector3D &b, const QVector3D &c, int polygon_id);
		bool addSide(const QVector3D &a, const QVector3D &b, int polygon_id);

		// Render
		void scanlineRender(int scanline);
		void initialFrameBuffer();
		bool activateSides(int scanline);
		void scan(int line);

		void updateActiveSideList();
		int findClosestPolygon(int x, int y);
		void drawLine(int x1, int x2, int y, QRgb color);

		// save render result
		void saveRenderResult();

		int m_width;
		int m_height;
		QRgb m_backgroundColor;
		float m_max_z;

		// Data structure of scanline algorithm.
		QVector<Polygon> m_polygonTable;
		QVector<QVector<Side>> m_sideTable;
		QVector<Side> m_activeSideList;
		QVector<QRgb> m_frame_buffer;

		// Vertex data.
		QVector<float> m_vertices;
		QVector<float> m_normals;
		QVector<unsigned int> m_indices;

		// Matrics for render.
		QVector3D m_camera_pos;
		QMatrix4x4 m_modelview;
		QMatrix4x4 m_projection;
		QRect m_viewport;

		QImage m_result;
	};
}