#include "ModelRender.h"

SpanningScanline::ModelRender::ModelRender(int width, int height) :
	m_width(width),
	m_height(height),
	m_polygonTable(height),
	m_sideTable(height),
	m_vertices(0),
	m_normals(0),
	m_indices(0)
{
}

void SpanningScanline::ModelRender::setBufferData(const QVector<float> &vertices, const QVector<float> &normals, const QVector<unsigned int> &indices)
{
	m_vertices = QVector<float>(vertices);
	m_normals = QVector<float>(normals);
	m_indices = QVector<unsigned int>(indices);
}

void SpanningScanline::ModelRender::render()
{
	if (!initialFromBufferData()) {
		return;
	}

	// TO DO ...
}

// TO DO ...
bool SpanningScanline::ModelRender::initialFromBufferData()
{
	QVector3D a, b, c;
	int count = 0;

	for (int i = 0; i < m_indices.size(); i += 3) {
		a = getProjectVertexFromBuffer(m_indices[i]);
		b = getProjectVertexFromBuffer(m_indices[i] + 1);
		c = getProjectVertexFromBuffer(m_indices[i] + 2);

		if (addPolygon(a, b, c, count)) {
			addSides(a, b, c, count);

			count++;
		}
	}

	return true;
}

QVector3D SpanningScanline::ModelRender::getProjectVertexFromBuffer(int index)
{
	int true_index = index * 3;

	QVector3D vertex;
	vertex.setX(m_vertices[true_index]);
	vertex.setY(m_vertices[true_index + 1]);
	vertex.setZ(m_vertices[true_index + 2]);

	return vertex.project(m_modelview, m_projection, m_viewport);
}

bool SpanningScanline::ModelRender::addPolygon(const QVector3D & a, const QVector3D & b, const QVector3D & c, int count)
{
	int maxY = (int)std::max(std::max(a.y(), b.y()), c.y());
	int minY = (int)std::min(std::min(a.y(), b.y()), c.y());

	if (maxY < 0 || minY >= m_height) {  // out of screen
		return false;
	}

	if (maxY < m_height) {  // maxY is in the screen
		QVector3D normal = QVector3D::normal((a - b), (a - c));

		// Add to polygon table
		Polygon p;
		p.a = normal.x();
		p.b = normal.y();
		p.c = normal.z();
		p.cross_y = maxY - minY;
		p.id = count;

		m_polygonTable[maxY].push_back(p);
	}
	else {
		// TO DO ...
	}

	return true;
}

bool SpanningScanline::ModelRender::addSides(const QVector3D & a, const QVector3D & b, const QVector3D & c, int polygon_id)
{
	addSide(a, b, polygon_id);
	addSide(a, c, polygon_id);
	addSide(b, c, polygon_id);

	return true;
}

bool SpanningScanline::ModelRender::addSide(const QVector3D & a, const QVector3D & b, int polygon_id)
{
	if (std::abs(a.y() - b.y()) < 1e-5) {  // ignore two close point
		return false;
	}

	QVector3D upper_vertex, lower_vertex;
	if (a.y() > b.y()) {
		upper_vertex = a;
		lower_vertex = b;
	}
	else if (a.y() < b.y()) {
		upper_vertex = b;
		lower_vertex = a;
	}

	int max_y = upper_vertex.y(), min_y = lower_vertex.y();
	int x_of_max_y = upper_vertex.x(), x_of_min_y = lower_vertex.x();

	if (max_y >= m_height) {  // the upper vertex out of screen
		// TO DO ...

		return false;
	}
	else {
		Side side;
		side.cross_y = max_y - min_y;
		side.delta_x = -(x_of_max_y - x_of_min_y) / (max_y - min_y);
		side.polygon_id = polygon_id;
		side.x = upper_vertex.x();
		side.z = upper_vertex.z();

		m_sideTable[max_y].push_back(side);
	}

	return true;
}

