#pragma once

#include <QVector>
#include <QMatrix4x4>

namespace SpanningScanline {
	struct Color {
		int r, g, b;
		float a;

		Color() {
			r = 255;
			g = b = 0;

		}
	};

	struct Polygon {
		float a, b, c;  // Normal vector of the polygon n = (a, b, c)
		// double d;	// ax + by + cz + d = 0
		int cross_y;	// The number of scanlines crossed by the polygon
		// Color color;
		unsigned int id;

		Polygon() {
			a = b = c = cross_y = 0;
			id = -1;
		}
	};

	struct Side {
		float delta_x;		// dy / dx = k, dx = dy / k, dy = -1, dx = - 1 / k
		int cross_y;		// The number of scanlines crossed by the side
		unsigned int polygon_id;
		int x;			// the x value of point y_max
		float z;			// the z value of point y_max
	};

	class ModelRender
	{
	public:
		ModelRender(int width, int height);
		void setBufferData(const QVector<float> &vertices, const QVector<float> &normals, const QVector<unsigned int> &indices);
		void render();

	private:
		bool initialFromBufferData();
		QVector3D getProjectVertexFromBuffer(int index);
		bool addPolygon(const QVector3D &a, const QVector3D &b, const QVector3D &c, int polygon_id);
		bool addSides(const QVector3D &a, const QVector3D &b, const QVector3D &c, int polygon_id);
		bool addSide(const QVector3D &a, const QVector3D &b, int polygon_id);

		int m_width;
		int m_height;

		QVector<QVector<Polygon>> m_polygonTable;
		QVector<QVector<Side>> m_sideTable;

		// Vertex data.
		QVector<float> m_vertices;
		QVector<float> m_normals;
		QVector<unsigned int> m_indices;

		// Matrics for render.
		QMatrix4x4 m_modelview;
		QMatrix4x4 m_projection;
		QRect m_viewport;

	};
}