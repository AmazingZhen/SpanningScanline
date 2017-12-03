#pragma once

#include <QVector>
#include <QMatrix4x4>
#include <QImage>

#include <iostream>

using namespace std;

namespace SpanningScanline {
	struct Polygon {
		float a, b, c;  // Normal vector of the polygon n = (a, b, c)
		// double d;	// ax + by + cz + d = 0
		int cross_y;	// The number of scanlines crossed by the polygon
		// Color color;
		unsigned int id;
	};

	struct Side {
		unsigned int polygon_id;
		float delta_x;		// dy / dx = k, dx / dy = 1 / k
		int cross_y;		// The number of scanlines crossed by the side
		float x;				// the x value of point y_max
		float z;			// the z value of point y_max

		void print() {
			cout << "polygon_id :" << polygon_id << endl;
			cout << "delta_x :" << delta_x << endl;
			cout << "cross_y :" << cross_y << endl;
			cout << "x :" << x << endl;
			cout << "z :" << z << endl << endl;
		}
	};

	struct SidePair {
		// 'left' side of polygon cross scanline
		float x_l;			// x value of intersection
		float dx_l;			// delta_x
		float cross_y_l;	// cross_y

		// 'right' side of polygon cross scanline
		float x_r, dx_r, cross_y_r;

		float z_l;			// z value of intersection
		float dz_l_along_x;	// delta_z along x axis, dz / dx
		float dz_r_along_y;	// delta_z along y axis, dz / dy

		int polygon_id;

		void print() {
			cout << "x_l :"			<< x_l << endl;
			cout << "dx_l :"		<< dx_l << endl;
			cout << "cross_y_l :"	<< cross_y_l << endl;
			cout << "x_r :"			<< x_r << endl;
			cout << "dx_r :"		<< dx_r << endl;
			cout << "cross_y_r :"	<< cross_y_r << endl;
			cout << "z_l :"			<< z_l << endl;
			cout << "dz_l_along_x :" << dz_l_along_x << endl;
			cout << "dz_r_along_y :" << dz_r_along_y << endl << endl;
		}
	};

	class ModelRender
	{
	public:
		ModelRender(QRgb backgroundColor);
		void setBufferData(const QVector<float> &vertices, const QVector<float> &normals, const QVector<unsigned int> &indices);
		void render();
		QImage getRenderResult();

		void setCameraPos(QVector3D pos);
		void setWindowSize(int width, int height);

	private:
		// Initial data structure of scanline algorithm.
		bool initialPolygonTableAndSideTable();
		QVector3D getProjectVertexFromBuffer(int index);
		bool addPolygon(const QVector3D &a, const QVector3D &b, const QVector3D &c, int polygon_id);
		bool addSides(const QVector3D &a, const QVector3D &b, const QVector3D &c, int polygon_id);
		bool addSide(const QVector3D &a, const QVector3D &b, int polygon_id);

		// Render
		void scanlineRender(int scanline);
		void initialFrameBuffer();
		void initialZBbuffer();
		bool activePolygonsAndSides(int scanline);
		bool activeSides(int scanline, const Polygon &p);
		bool activeSidePair(const Side &a, const Side &b, const Polygon &p);
		void scan(int line);
		void updateSidePair(QList<SidePair>::iterator &sp_iter, int scanline);

		// save render result
		void saveRenderResult();

		int m_width;
		int m_height;
		QRgb m_backgroundColor;
		float m_min_z;

		// Data structure of scanline algorithm.
		QVector<QVector<Polygon>> m_polygonTable;
		QVector<QVector<Side>> m_sideTable;
		QList<Polygon> m_activePolygonTable;
		QList<SidePair> m_activeSidePairTable;
		QVector<float> m_z_buffer;
		QVector<QVector<QRgb>> m_frame_buffer;

		// Vertex data.
		QVector<float> m_vertices;
		QVector<float> m_normals;
		QVector<unsigned int> m_indices;

		// Matrics for render.
		QMatrix4x4 m_modelview;
		QMatrix4x4 m_projection;
		QRect m_viewport;

		QImage m_result;
	};
}