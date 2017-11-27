#include "ModelRender.h"

SpanningScanline::ModelRender::ModelRender() :
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

bool SpanningScanline::ModelRender::initialFromBufferData()
{
	// TO DO ...

	return true;
}
