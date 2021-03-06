#include "ModelRender.h"

SpanningScanline::ModelRender::ModelRender(QRgb backgroundColor) :
	m_backgroundColor(backgroundColor),
	m_max_z(100.f),
	m_frame_buffer(0),
	m_width(0),
	m_height(0)
{
}

void SpanningScanline::ModelRender::setBufferData(const QVector<float> &vertices, const QVector<float> &normals, const QVector<unsigned int> &indices)
{
	m_vertices = QVector<float>(vertices);
	m_normals = QVector<float>(normals);
	m_indices = QVector<unsigned int>(indices);
}

bool SpanningScanline::ModelRender::render()
{
	static bool isRendering = false;

	if (isRendering) {
		return false;
	}

	if (!initialPolygonTableAndSideTable()) {
		return false;
	}

	isRendering = true;

	initialFrameBuffer();

	for (int curScanline = m_height - 1; curScanline >= 0; curScanline--) {
		scanlineRender(curScanline);
	}

	saveRenderResult();

	isRendering = false;

	return true;
}

QImage SpanningScanline::ModelRender::getRenderResult()
{
	return m_result;
}

void SpanningScanline::ModelRender::setCameraPos(const QVector3D &pos)
{
	m_camera_pos = pos;

	m_modelview = QMatrix4x4();
	m_modelview.lookAt(pos, QVector3D(0.f, 0.f, 0.f), QVector3D(0.f, 1.f, 0.f));
}

void SpanningScanline::ModelRender::setWindowSize(int width, int height)
{
	m_width = width;
	m_height = height;

	m_sideTable = QVector<QVector<Side>>(height);

	m_frame_buffer = QVector<QRgb>(width * height);

	m_result = QImage(width, height, QImage::Format_RGB32);

	m_projection = QMatrix4x4();
	m_projection.perspective(70.0, width / height, 0.1f, 100.f);

	m_viewport = QRect(0, 0, width, height);
}

bool SpanningScanline::ModelRender::initialPolygonTableAndSideTable()
{
	m_polygonTable.clear();
	for (int i = 0; i < m_height; i++) {
		m_sideTable[i].clear();
	}

	m_activeSideList.clear();

	int count = 0;

	//#pragma omp parallel
	{
		QVector3D a, b, c, polygon_pos;
		QVector3D a_normal, b_normal, c_normal, polygon_normal;
		QVector3D a_project, b_project, c_project;
		QVector3D view;
		float factor = 0.f;

		//#pragma omp for
		for (int i = 0; i < m_indices.size(); i += 3) {
			a = getVertexFromBuffer(m_indices[i]);
			b = getVertexFromBuffer(m_indices[i + 1]);
			c = getVertexFromBuffer(m_indices[i + 2]);

			a_normal = getNormalFromBuffer(m_indices[i]);
			b_normal = getNormalFromBuffer(m_indices[i + 1]);
			c_normal = getNormalFromBuffer(m_indices[i + 2]);

			// Get color factor by normal * view
			polygon_pos = (a + b + c) / 3;
			view = (m_camera_pos - polygon_pos).normalized();
			polygon_normal = ((a_normal + b_normal + c_normal) / 3).normalized();
			factor = QVector3D::dotProduct(polygon_normal, view);

			//if (factor <= 0.f) {
				//continue;
			//}

			a_project = a.project(m_modelview, m_projection, m_viewport);
			b_project = b.project(m_modelview, m_projection, m_viewport);
			c_project = c.project(m_modelview, m_projection, m_viewport);

			//#pragma omp critical
			{
				if (addPolygon(a_project, b_project, c_project, factor, count)) {
					addSides(a_project, b_project, c_project, count);

					count++;
				}
			}
		}
	}

	return true;
}

QVector3D SpanningScanline::ModelRender::getVertexFromBuffer(int index)
{
	int true_index = index * 3;
	QVector3D vertex(m_vertices[true_index], m_vertices[true_index + 1], m_vertices[true_index + 2]);
	return vertex;
}

QVector3D SpanningScanline::ModelRender::getNormalFromBuffer(int index)
{
	int true_index = index * 3;
	QVector3D normal(m_normals[true_index], m_normals[true_index + 1], m_normals[true_index + 2]);
	return normal;
}

bool SpanningScanline::ModelRender::addPolygon(const QVector3D & a, const QVector3D & b, const QVector3D & c, float factor, int count)
{
	int maxY = (int)std::max(std::max(a.y(), b.y()), c.y());
	int minY = (int)std::min(std::min(a.y(), b.y()), c.y());

	if (maxY < 0 || minY >= m_height) {  // totally out of screen
		return false;
	}

	/*
	if (maxY >= m_height) {  // partly out of screen
		// TO DO ...
		// Actually it needs triangle clipping
		return false;
	}
	*/

	QVector3D normal = QVector3D::normal((a - b), (a - c));
	if (normal.z() == 0) {
		// printf("zero plane");
		return false;
	}

	// Add to polygon table
	Polygon p;
	p.id = count;
	p.a = normal.x();
	p.b = normal.y();
	p.c = normal.z();
	p.d = -(p.a * a.x() + p.b * a.y() + p.c * a.z());
	p.cross_y = maxY - minY;

	p.color = qRgb(factor * 255, factor * 255, factor * 255);

	m_polygonTable.push_back(p);

	return true;
}

bool SpanningScanline::ModelRender::addSides(const QVector3D & a, const QVector3D & b, const QVector3D & c, int polygon_id)
{
	addSide(a, b, polygon_id);
	addSide(a, c, polygon_id);
	addSide(b, c, polygon_id);

	return true;
}

bool SpanningScanline::ModelRender::addSide(const QVector3D &a, const QVector3D &b, int polygon_id)
{
	if (std::abs((int)a.y() - (int)b.y()) == 0) {  // ignore side parallel to scanline
		return false;
	}

	QVector3D upper_vertex, lower_vertex;
	if (a.y() > b.y()) {
		upper_vertex = a;
		lower_vertex = b;
	}
	else {
		upper_vertex = b;
		lower_vertex = a;
	}

	int max_y = upper_vertex.y(), min_y = lower_vertex.y();
	float x_of_max_y = upper_vertex.x(), x_of_min_y = lower_vertex.x();

	if (max_y < 0 || min_y >= m_height - 1) {  // upper vertex out of screen bottom or lower vertex out of top
		return false;
	}

	Side side;
	side.cross_y = max_y - min_y;
	side.delta_x = -(upper_vertex.x() - lower_vertex.x()) / (upper_vertex.y() - lower_vertex.y());
	side.polygon_id = polygon_id;
	side.x = upper_vertex.x();

	// If the upper vertex out of screen top, we 'cut' this side
	while (max_y >= m_height) {
		side.cross_y--;
		side.x += side.delta_x;
		max_y--;
	}

	m_sideTable[max_y].push_back(side);

	return true;
}

void SpanningScanline::ModelRender::scanlineRender(int scanline)
{
	int a = 1;
	if (scanline == m_height - 1) {
		a = 0;
	}

	activateSides(scanline);
	scan(scanline);
	updateActiveSideList();
}

void SpanningScanline::ModelRender::initialFrameBuffer()
{
	#pragma omp parallel for
	for (int i = 0; i < m_height * m_width; i++) {
		m_frame_buffer[i] = m_backgroundColor;
	}
}

bool SpanningScanline::ModelRender::activateSides(int scanline)
{
	for (const Side &s : m_sideTable[scanline]) {
		m_activeSideList.push_back(s);
	}

	qSort(m_activeSideList.begin(), m_activeSideList.end(), [](const Side &a, const Side &b) {
		return a.x < b.x;
	});

	return true;
}

void SpanningScanline::ModelRender::scan(int line)
{
	auto s_iter_left = m_activeSideList.begin();

	QMap<int, Polygon> activePolygonMap;
	while (s_iter_left != m_activeSideList.end() && s_iter_left->x < m_width) {
		Side &s_left = *s_iter_left;

		// Update activePolygonList
		auto p_iter = activePolygonMap.find(s_left.polygon_id);
		if (p_iter == activePolygonMap.end()) {
			activePolygonMap[s_left.polygon_id] = m_polygonTable[s_left.polygon_id];
		}
		else {
			activePolygonMap.erase(p_iter);
		}

		auto s_iter_right = s_iter_left + 1;

		if (s_iter_right != m_activeSideList.end()) {
			Side &s_right = *s_iter_right;
			QRgb color = qRgb(255, 255, 255);

			if (activePolygonMap.size() > 1) {  // find closest polygon
				float x = (s_left.x + s_right.x) / 2.f;
				float min_z = m_max_z;
				int closestPolygonId = -1;

				for (auto &p : activePolygonMap) {
					float z = -(p.a * x + p.b * line + p.d) / p.c;
					if (z < min_z) {
						min_z = z;
						closestPolygonId = p.id;
					}
				}

				if (closestPolygonId != -1) {
					color = m_polygonTable[closestPolygonId].color;
				}
			} else if (!activePolygonMap.empty()) {
				color = activePolygonMap.begin()->color;
			}
			else {
				color = m_backgroundColor;
			}

			drawLine(s_left.x, s_right.x, line, color);
		}

		s_iter_left = s_iter_right;
	}
}

void SpanningScanline::ModelRender::updateActiveSideList()
{
	auto s_iter = m_activeSideList.begin();

	while (s_iter != m_activeSideList.end()) {
		Side &s = *s_iter;

		s.cross_y--;
		if (s.cross_y <= 0) {
			s_iter = m_activeSideList.erase(s_iter);
			continue;
		}

		s.x += s.delta_x;

		s_iter++;
	}
}

int SpanningScanline::ModelRender::findClosestPolygon(int x, int y)
{
	return 0;
}

void SpanningScanline::ModelRender::drawLine(int x1, int x2, int y, QRgb color)
{
	int offset = (m_height - 1 - y) * m_width;

	for (int x = max(0, x1); x < min(x2, m_width); x++) {
		m_frame_buffer[offset + x] = color;
	}
}

void SpanningScanline::ModelRender::saveRenderResult()
{
	/*
	#pragma omp parallel for
	for (int r = 0; r < m_height; r++) {
		for (int c = 0; c < m_width; c++) {
			m_result.setPixel(c, m_height - 1 - r, m_frame_buffer[r][c]);
		}
	}
	*/

	QRgb *st = (QRgb*)m_result.bits();
	const int pixelCount = m_result.width() * m_result.height();

	#pragma omp parallel for
	for (int p = 0; p < pixelCount; p++) {
		// st[p] has an individual pixel
		st[p] = m_frame_buffer[p];
	}
}