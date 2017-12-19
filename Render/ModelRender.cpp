#include "ModelRender.h"

SpanningScanline::ModelRender::ModelRender(QRgb backgroundColor) :
	m_backgroundColor(backgroundColor),
	m_max_z(100.f),
	m_frameCount(0)
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
	if (!initialPolygonTableAndSideTable()) {
		return;
	}

	initialFrameBuffer();

	for (int curScanline = m_height - 1; curScanline >= 0; curScanline--) {
		scanlineRender(curScanline);
	}

	saveRenderResult();
	m_frameCount++;
	printf("Render %d frame finish\n", m_frameCount);
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

	//m_polygonTable = QVector<Polygon>();
	//m_sideTable = QVector<QVector<Side>>(m_height);

	m_z_buffer = QVector<float>(m_width);

	m_frame_buffer = QVector<QVector<QRgb>>(m_height, QVector<QRgb>(m_width));
	m_result = QImage(m_width, m_height, QImage::Format_RGB32);

	m_projection = QMatrix4x4();
	m_projection.perspective(70.0, m_width / m_height, 0.1f, 100.f);

	m_viewport = QRect(0, 0, m_width, m_height);
}

void SpanningScanline::ModelRender::switchPolygon(bool up)
{
	m_curPolygonId += up ? 1 : -1;
	m_curPolygonId += m_indices.size() / 3;
	m_curPolygonId %= m_indices.size() / 3;

	printf("%d\n", m_curPolygonId);
}

// TO DO ...
bool SpanningScanline::ModelRender::initialPolygonTableAndSideTable()
{
	m_polygonTable = QVector<Polygon>();
	m_sideTable = QVector<QVector<Side>>(m_height);
	m_activePolygonTable.clear();
	m_activeSidePairTable.clear();

	QVector3D a, b, c, polygon_pos;
	QVector3D a_normal, b_normal, c_normal, polygon_normal;
	QVector3D a_project, b_project, c_project;
	QVector3D view;
	float factor = 0.f;
	int count = 0;

	for (int i = 0; i < m_indices.size(); i += 3) {
		a = getVertexFromBuffer(m_indices[i]);
		b = getVertexFromBuffer(m_indices[i + 1]);
		c = getVertexFromBuffer(m_indices[i + 2]);
		
		a_normal = getNormalFromBuffer(m_indices[i]);
		b_normal = getNormalFromBuffer(m_indices[i + 1]);
		c_normal = getNormalFromBuffer(m_indices[i + 2]);

		// Get color factor by normal * view
		// polygon_pos = (a + b + c) / 3;
		view = (m_camera_pos - polygon_pos).normalized();
		polygon_normal = ((a_normal + b_normal + c_normal) / 3).normalized();
		factor = QVector3D::dotProduct(polygon_normal, view);

		if (factor <= 0.f) {
			continue;
		}

		// printf("%f\n\n", factor);
		/*
		printf("%d\n", count);
		qDebug() << origin_polygon_normal;
		qDebug() << forward;
		printf("%f\n\n", factor);
		*/

		a_project = a.project(m_modelview, m_projection, m_viewport);
		b_project = b.project(m_modelview, m_projection, m_viewport);
		c_project = c.project(m_modelview, m_projection, m_viewport);

		if (addPolygon(a_project, b_project, c_project, factor, count)) {
			addSides(a_project, b_project, c_project, count);

			count++;
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

	if (maxY < 0 || minY >= m_height) {  // out of screen
		return false;
	}

	if (maxY >= m_height) {  // maxY is out the screen
		// TO DO ...

		return false;
	}

	QVector3D normal = QVector3D::normal((a - b), (a - c));

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
	if (std::abs(a.y() - b.y()) < 0.2f) {  // ignore side parallel to scanline
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

	if (max_y >= m_height || max_y < 0) {  // the upper vertex out of screen
		// TO DO ...

		return false;
	}

	Side side;
	side.cross_y = max_y - min_y;
	side.delta_x = -(upper_vertex.x() - lower_vertex.x()) / (upper_vertex.y() - lower_vertex.y());
	side.polygon_id = polygon_id;
	side.x = upper_vertex.x();
	side.z = upper_vertex.z();

	m_sideTable[max_y].push_back(side);

	return true;
}

void SpanningScanline::ModelRender::scanlineRender(int scanline)
{
	initialZBbuffer();
	activeSides(scanline);
	scan(scanline);
	updateActiveSideList();
}

void SpanningScanline::ModelRender::initialFrameBuffer()
{
	for (int i = 0; i < m_height; i++) {
		for (int j = 0; j < m_width; j++) {
			m_frame_buffer[i][j] = m_backgroundColor;
		}
	}
}

void SpanningScanline::ModelRender::initialZBbuffer()
{
	for (int i = 0; i < m_width; i++) {
		m_z_buffer[i] = m_max_z;
	}
}

bool SpanningScanline::ModelRender::activePolygonsAndSides(int scanline)
{


	return true;
}

bool SpanningScanline::ModelRender::activeSides(int scanline)
{
	if (m_sideTable[scanline].isEmpty()) {
		return false;
	}

	for (const Side &s : m_sideTable[scanline]) if (!singlePolygon || s.polygon_id == m_curPolygonId) {
		m_activeSideList.push_back(s);
	}

	qSort(m_activeSideList.begin(), m_activeSideList.end(), [](const Side &a, const Side &b) {
		return a.x < b.x;
	});

	return true;
}

bool SpanningScanline::ModelRender::activeSidePair(const Side &a, const Side &b, const Polygon &p)
{
	Side left, right;

	if (a.x + a.delta_x < b.x + b.delta_x) {
		left = a;
		right = b;
	}
	else {
		left = b;
		right = a;
	}

	SidePair sidePair;
	sidePair.polygon_id = left.polygon_id;

	sidePair.x_l = left.x;
	sidePair.dx_l = left.delta_x;
	sidePair.cross_y_l = left.cross_y;

	sidePair.x_r = right.x;
	sidePair.dx_r = right.delta_x;
	sidePair.cross_y_r = right.cross_y;

	sidePair.z_l = left.z;

	// ax + by + cz + d = 0
	// dz / dx = -a / c, dz / dy = -b / c
	if (p.c == 0) {
		sidePair.dz_along_x = sidePair.dz_along_y = 0;
	}
	else {
		sidePair.dz_along_x = -p.a / p.c;
		sidePair.dz_along_y = p.b / p.c;
	}

	sidePair.color = p.color;

	m_activeSidePairTable.push_back(sidePair);

	return true;
}

float getZ(const SpanningScanline::Polygon &p, int x, int y) {
	if (p.c != 0.f) {
		return -(p.a * x + p.b * y + p.d) / p.c;
	}
	
	return 100.f;
}

void SpanningScanline::ModelRender::scan(int line)
{
	auto s_iter_left = m_activeSideList.begin();

	QList<Polygon> activePolygonList;
	while (s_iter_left != m_activeSideList.end() && s_iter_left->x < m_width) {
		Side &s_left = *s_iter_left;
		// Update activePolygonList
		auto p_iter = activePolygonList.begin();

		bool flagIn = true;
		while (p_iter != activePolygonList.end()) {
			if (p_iter->id == s_left.polygon_id) {
				// If find, it means that scanline is out of the polygon
				activePolygonList.erase(p_iter);
				flagIn = false;
				break;
			}
			p_iter++;
		}
		if (flagIn) {
			activePolygonList.push_back(m_polygonTable[s_left.polygon_id]);
		}

		auto s_iter_right = s_iter_left + 1;

		if (s_iter_right != m_activeSideList.end()) {
			Side &s_right = *s_iter_right;
			QRgb color = Qt::red;

			if (activePolygonList.size() > 1) {  // find closest polygon
				int x = (s_left.x + s_right.x) / 2;
				float min_z = m_max_z;
				int closestPolygonId = -1;
				for (auto &p : activePolygonList) {
					float z = getZ(p, x, line);
					if (z < min_z) {
						min_z = z;
						closestPolygonId = p.id;
					}
				}
				if (closestPolygonId != -1) {
					color = m_polygonTable[closestPolygonId].color;
				}
			} else if (!activePolygonList.empty()) {
				color = activePolygonList[0].color;	
			}

			drawLine(s_left.x, s_right.x, line, color);
		}

		s_iter_left = s_iter_right;
	}
}

void SpanningScanline::ModelRender::updateSidePair(QList<SidePair>::iterator &sp_iter, int scanline)
{
	SidePair &sp = *sp_iter;

	/*
	if (sp.polygon_id == 2) {
		cout << scanline << endl;
		sp.print();
	}
	//*/

	if (sp.cross_y_l == 0 && sp.cross_y_r == 0) {  // two side both out of scanline
		// TO DO ...

		sp_iter = m_activeSidePairTable.erase(sp_iter);
		return;
	}
	else if (sp.cross_y_l == 0 || sp.cross_y_r == 0) {    // one side out of scanline
		Side newSide;
		bool isFound = false;

		for (const Side& s : m_sideTable[scanline]) {
			if (s.polygon_id == sp.polygon_id) {
				newSide = s;
				isFound = true;
				break;
			}
		}

		if (!isFound) {
			sp_iter = m_activeSidePairTable.erase(sp_iter);
			return;
		}

		// replace one of side pair
		if (sp.cross_y_l == 0) {  // left side
			sp.x_l = newSide.x;
			sp.cross_y_l = newSide.cross_y;
			sp.dx_l = newSide.delta_x;

			sp.z_l = newSide.z;
		}
		else {  // right side
			sp.x_r = newSide.x;
			sp.cross_y_r = newSide.cross_y;
			sp.dx_r = newSide.delta_x;
		}
	}

	sp.cross_y_l--;
	sp.cross_y_r--;

	// dx / dy = d_x,  dy = -1(one pixel down), so dx = -1 / k
	sp.x_l += sp.dx_l;
	sp.x_r += sp.dx_r;

	// dz / dx = -a / c
	// dz / dy = -b / c, dy = -1
	sp.z_l += sp.dz_along_x * sp.dx_l + sp.dz_along_y;

	sp_iter++;
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

	qSort(m_activeSideList.begin(), m_activeSideList.end(), [](const Side &a, const Side &b) {
		return a.x < b.x;
	});
}

int SpanningScanline::ModelRender::findClosestPolygon(int x, int y)
{
	return 0;
}

void SpanningScanline::ModelRender::drawLine(int x1, int x2, int y, QRgb color)
{
	for (int x = max(0, x1); x < min(x2, m_width); x++) {
		m_frame_buffer[y][x] = color;
	}
}

void SpanningScanline::ModelRender::saveRenderResult()
{
	for (int r = 0; r < m_height; r++) {
		for (int c = 0; c < m_width; c++) {
			m_result.setPixel(c, m_height - 1 - r, m_frame_buffer[r][c]);
		}
	}
}