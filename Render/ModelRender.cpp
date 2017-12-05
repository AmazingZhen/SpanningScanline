#include "ModelRender.h"

SpanningScanline::ModelRender::ModelRender(QRgb backgroundColor) :
	m_backgroundColor(backgroundColor),
	m_min_z(-1000.f)
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

	printf("Render finish\n");
}

QImage SpanningScanline::ModelRender::getRenderResult()
{
	return m_result;
}

void SpanningScanline::ModelRender::setCameraPos(QVector3D pos)
{
	m_modelview = QMatrix4x4();
	m_modelview.lookAt(pos, QVector3D(0.f, 0.f, 0.f), QVector3D(0.f, 1.f, 0.f));
}

void SpanningScanline::ModelRender::setWindowSize(int width, int height)
{
	m_width = width;
	m_height = height;

	m_polygonTable = QVector<QVector<Polygon>>(m_height);
	m_sideTable = QVector<QVector<Side>>(m_height);

	m_z_buffer = QVector<float>(m_width);

	m_frame_buffer = QVector<QVector<QRgb>>(m_height, QVector<QRgb>(m_width));
	m_result = QImage(m_width, m_height, QImage::Format_RGB32);

	m_projection = QMatrix4x4();
	m_projection.perspective(70.0, m_width / m_height, 0.1f, 100.f);

	m_viewport = QRect(0, 0, m_width, m_height);
}

// TO DO ...
bool SpanningScanline::ModelRender::initialPolygonTableAndSideTable()
{
	m_polygonTable = QVector<QVector<Polygon>>(m_height);
	m_sideTable = QVector<QVector<Side>>(m_height);
	m_activePolygonTable.clear();
	m_activeSidePairTable.clear();

	QVector3D a, b, c;
	QVector3D a_normal, b_normal, c_normal, origin_polygon_normal;
	QVector3D a_project, b_project, c_project;
	QVector3D forward = QVector3D(m_modelview.row(2)[0], m_modelview.row(2)[1], m_modelview.row(2)[2]).normalized();
	float factor = 0.f;
	int count = 0;

	for (int i = 0; i < m_indices.size(); i += 3) {
		//printf("Face %d:\n", i / 3);
		
		a = getVertexFromBuffer(m_indices[i]);
		b = getVertexFromBuffer(m_indices[i + 1]);
		c = getVertexFromBuffer(m_indices[i + 2]);
		
		a_normal = getNormalFromBuffer(m_indices[i]);
		b_normal = getNormalFromBuffer(m_indices[i + 1]);
		c_normal = getNormalFromBuffer(m_indices[i + 2]);

		// Get color factor by normal * forward
		origin_polygon_normal = ((a_normal + b_normal + c_normal) / 3).normalized();
		factor = QVector3D::dotProduct(origin_polygon_normal, forward);
		if (factor <= 0.f) {
			continue;
		}
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

		//printf("\n");
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

	if (maxY < m_height) {  // maxY is in the screen
		QVector3D normal = QVector3D::normal((a - b), (a - c));

		// Add to polygon table
		Polygon p;
		p.id = count;
		p.a = normal.x();
		p.b = normal.y();
		p.c = normal.z();
		p.cross_y = maxY - minY;

		p.color = qRgb(factor * 255, factor * 255, factor * 255);

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
	side.delta_x = (x_of_max_y - x_of_min_y) / (max_y - min_y);
	side.polygon_id = polygon_id;
	side.x = upper_vertex.x();
	side.z = upper_vertex.z();

	m_sideTable[max_y].push_back(side);

	/*
	if (side.polygon_id == 2) {
		cout << max_y << endl;
		side.print();
	}
	//*/

	return true;
}

void SpanningScanline::ModelRender::scanlineRender(int scanline)
{
	initialZBbuffer();
	activePolygonsAndSides(scanline);
	scan(scanline);
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
		m_z_buffer[i] = m_min_z;
	}
}

bool SpanningScanline::ModelRender::activePolygonsAndSides(int scanline)
{
	if (m_polygonTable[scanline].isEmpty()) {
		return false;
	}

	for (const Polygon &p : m_polygonTable[scanline]) {
		m_activePolygonTable.push_back(p);
		
		activeSides(scanline, p);
	}

	return true;
}

bool SpanningScanline::ModelRender::activeSides(int scanline, const Polygon &p)
{
	if (m_sideTable[scanline].isEmpty()) {
		return false;
	}

	int count = 0;
	Side sides[2];
	for (const Side &s : m_sideTable[scanline]) if (s.polygon_id == p.id){
		sides[count] = s;
		count++;

		if (count == 2) {
			break;
		}
	}

	if (count < 2) {
		// TO DO ...

		return false;
	}

	return activeSidePair(sides[0], sides[1], p);
}

bool SpanningScanline::ModelRender::activeSidePair(const Side &a, const Side &b, const Polygon &p)
{
	Side left, right;

	if (a.x - a.delta_x < b.x - b.delta_x) {
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

	sidePair.color = p.color;

	// ax + by + cz + d = 0
	// dz / dx = -a / c, dz / dy = -b / c
	if (p.c == 0) {
		sidePair.dz_l_along_x = sidePair.dz_r_along_y = 0;
	}
	else {
		sidePair.dz_l_along_x = -p.a / p.c;
		sidePair.dz_r_along_y = -p.b / p.c;
	}

	m_activeSidePairTable.push_back(sidePair);

	return true;
}

void SpanningScanline::ModelRender::scan(int line)
{
	if (m_activeSidePairTable.isEmpty()) {
		return;
	}
	
	//QVector3D view = (m_projection * QVector4D(m_modelview.row(2)[0], m_modelview.row(2)[1], m_modelview.row(2)[2], 0.f)).toVector3D().normalized();

	//printf("curline: %d\n", line);

	auto sp_iter = m_activeSidePairTable.begin();

	while (sp_iter != m_activeSidePairTable.end()) {
		SidePair &sp = *sp_iter;	

		/*
		if (sp.polygon_id != 9) {
			sp_iter++;
			continue;
		}
		//*/

		float cur_z = sp.z_l;
		//float factor = std::abs(QVector4D::dotProduct(view, sp.normal));
		//int gray = 255 * factor;
		//qDebug() << sp.normal;
		//printf("factor: %f\n", factor);

		for (int x = sp.x_l; x <= sp.x_r; x++) {
			if (x < 0 || x >= m_width) {
				break;
			}

			if (cur_z > m_z_buffer[x]) {
				m_z_buffer[x] = cur_z;

				// Color  TO DO ...
				//m_frame_buffer[line][x] = (x < sp.x_r + 1 && x > sp.x_l + 1) ? m_backgroundColor : qRgb(255, 255, 255);
				m_frame_buffer[line][x] = sp.color;
				// m_frame_buffer[line][x] = qRgb(255, 255, 255);

				//m_frame_buffer[line][x] = qRgb(gray, gray, gray);
			}

			cur_z += sp.dz_l_along_x;
		}
		// printf("\n\n");

		updateSidePair(sp_iter, line);
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
	sp.x_l -= sp.dx_l;
	sp.x_r -= sp.dx_r;

	// dz / dx = -a / c
	// dz / dy = -b / c, dy = -1
	sp.z_l += sp.dz_l_along_x * (-sp.dx_l) + sp.dz_r_along_y * (-1);

	sp_iter++;
}

void SpanningScanline::ModelRender::saveRenderResult()
{
	for (int r = 0; r < m_height; r++) {
		for (int c = 0; c < m_width; c++) {
			m_result.setPixel(c, m_height - 1 - r, m_frame_buffer[r][c]);
		}
	}
}