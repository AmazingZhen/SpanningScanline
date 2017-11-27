#pragma once

#include <QVector>
#include <QMatrix4x4>

namespace SpanningScanline {
	class ModelRender
	{
	public:
		ModelRender();

		void setBufferData(const QVector<float> &vertices, const QVector<float> &normals, const QVector<unsigned int> &indices);

		void render();

	private:
		bool initialFromBufferData();

		// Vertex data.
		QVector<float> m_vertices;
		QVector<float> m_normals;
		QVector<unsigned int> m_indices;

		// Matrics for render.
		QMatrix4x4 modelview;
		QMatrix4x4 projection;
	};
}