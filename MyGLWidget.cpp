#include "MyGLWidget.h"
#include "tiny_obj_loader.h"

using namespace mf;

MyGLWidget::MyGLWidget(QWidget *parent)
	: QGLWidget(parent)
	, m_arcball(1920, 1080)
	, textureC(0)
	, m_indexBuf(QOpenGLBuffer::IndexBuffer)
	, indices(nullptr)
	, mfaces(0)
	, mframeid(0)
	, savePath("./")
{
	frames = 0;
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NoSystemBackground);
	setAutoBufferSwap(false);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateGL()));
	timer->start(10);

	pose = CameraPose(G.Intrinsic, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
}

MyGLWidget::~MyGLWidget()
{
	if (textureC)
		delete textureC;
	if (indices) {
		free(indices);
		indices = nullptr;
	}
	m_indexBuf.destroy();

	if (m_frameBuf)
		delete m_frameBuf;
}

const char* MyGLWidget::ReadShaderFile(const char* filepath) {
	FILE *fp = fopen(filepath, "rb");
	if (!fp) return NULL;
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *content = (char*)malloc(size + 1);
	fread(content, sizeof(char), size, fp);
	content[size] = '\0';
	fclose(fp);
	return (const char*)content;
}

bool MyGLWidget::readObj(const char *path) {
	std::vector<tinyobj::shape_t> objects;
	std::vector<tinyobj::material_t> mat_list;
	objects.clear();
	std::string err;
	bool ret = tinyobj::LoadObj(objects, mat_list, err, path);
	if (!err.empty()) std::cerr << err << std::endl;
	if (!ret) return false;

	mfaces = 0;
	mverts = 0;
	for (int i = 0; i < objects.size(); ++i) {
		mfaces += objects[i].mesh.indices.size() / 3;
		mverts += objects[i].mesh.positions.size() / 3;
	}

	//std::cout << "faces : " << mfaces << std::endl;
	if (indices) { free(indices); indices = nullptr; }
	indices = (GLushort*)malloc(sizeof(GLushort) * mfaces * 3);

	if (verts) { free(verts); verts = nullptr; }
	verts = (GLfloat*)malloc(sizeof(GLfloat) * mverts * 11);

    //if (normals) { free(normals); normals = nullptr; }
    //normals = (GLfloat*)malloc(sizeof(GLfloat) * mnormals * 3);

    //if (tangents) { free(tangents); tangents = nullptr; }
    //tangents = (GLfloat*)malloc(sizeof(GLfloat) * mverts * 3);

	int index = 0;
	vertices.clear();
	for (int i = 0; i < objects.size(); ++i) {
		for (int j = 0; j < objects[i].mesh.positions.size(); j+=3) {
			float x, y, z;
			x = (objects[i].mesh.positions[j]  + 300.0)/100.0 - 2.50;
			y = (objects[i].mesh.positions[j + 1] + 300.0)/100.0 - 2.50;
			z = (objects[i].mesh.positions[j + 2] + 600.0)/100.0 ;
			vertices << QVector3D(x, y, z);

			verts[index++] = x;
			verts[index++] = y;
			verts[index++] = z;
		}
	}

	vts.clear();
	for (int i = 0; i < objects.size(); ++i) {
		for (int j = 0; j < objects[i].mesh.texcoords.size(); j += 2) {
			float x, y;
			x = objects[i].mesh.texcoords[j];
			y = objects[i].mesh.texcoords[j + 1];
			vts << QVector2D(x, y);

			verts[index++] = x;
			verts[index++] = y;
		}
	}

    for (int i = 0; i < objects.size(); ++i) {
        for (int j = 0; j < objects[i].mesh.positions.size(); j += 3) {
            verts[index++] = objects[i].mesh.normals[j];
            verts[index++] = objects[i].mesh.normals[j + 1];
            verts[index++] = objects[i].mesh.normals[j + 2];
        }
    }

    int index_indices = 0;
    for (int i = 0; i < objects.size(); ++i) {
        for (int j = 0; j < objects[i].mesh.indices.size(); ++j) {
            indices[index_indices++] = objects[i].mesh.indices[j];
        }
    }

    //if (hasPBR && !vertex.empty() && !normal.empty() && !uv.empty() && !faces.empty())
    {
        // outputs
        QVector<QVector3D>  temp_tangents;

        temp_tangents.reserve(mfaces);

        for (auto i = 0; i < mfaces*3; i +=3)
        {
            unsigned int index0 = indices[i];
            unsigned int index1 = indices[i + 1];
            unsigned int index2 = indices[i + 2];
            QVector3D Qv0(verts[index0 * 3], verts[index0 * 3 + 1], verts[index0 * 3 + 2]);
            QVector3D Qv1(verts[index1 * 3], verts[index1 * 3 + 1], verts[index1 * 3 + 2]);
            QVector3D Qv2(verts[index2 * 3], verts[index2 * 3 + 1], verts[index2 * 3 + 2]);
            QVector2D Quv0(verts[mverts * 3 + index0 * 3], verts[mverts * 3 + index0 * 3 + 1]);
            QVector2D Quv1(verts[mverts * 3 + index1 * 3], verts[mverts * 3 + index1 * 3 + 1]);
            QVector2D Quv2(verts[mverts * 3 + index2 * 3], verts[mverts * 3 + index2 * 3 + 1]);

            // Edges of the triangle : position delta
            QVector3D deltaPos1 = Qv1 - Qv0;
            QVector3D deltaPos2 = Qv2 - Qv0;

            // UV delta
            QVector2D deltaUV1 = Quv1 - Quv0;
            QVector2D deltaUV2 = Quv2 - Quv0;

            QVector3D tangent = (deltaPos1 * deltaUV2.y() - deltaPos2 * deltaUV1.y()) /
                (deltaUV1.x() * deltaUV2.y() - deltaUV1.y() * deltaUV2.x());

            temp_tangents.insert(temp_tangents.end(), 3, tangent);
        }

        QVector<QVector3D> ave_tangents(mverts, QVector3D(.0f, .0f, .0f));

        for (auto i = 0; i < mfaces*3; ++i)
        {
            ave_tangents[indices[i]] += temp_tangents[i];
        }

        for (auto i = 0; i < mverts; ++i)
        {
            auto normal_tegent = ave_tangents[i].normalized();
            verts[index++] = normal_tegent.x();
            verts[index++] = normal_tegent.y();
            verts[index++] = normal_tegent.z();
        }
    }

	m_indexBuf.destroy();
	m_indexBuf.create();
	m_indexBuf.bind();
	m_indexBuf.allocate(indices, sizeof(GLushort)*mfaces * 3);

	m_vertexBuf.destroy();
	m_vertexBuf.create();
	m_vertexBuf.bind();
	m_vertexBuf.allocate(verts, sizeof(GLfloat)*mverts * 11);//5+3+3
	m_vertexBuf.release();

}

bool MyGLWidget::readTexture(const char *path) {
	if (textureC)
		delete textureC;

	textureC = new QOpenGLTexture(QImage(path).mirrored());
	textureC->setMinificationFilter(QOpenGLTexture::Linear);
	textureC->setMagnificationFilter(QOpenGLTexture::Linear);
	textureC->setWrapMode(QOpenGLTexture::Repeat);
	return true;
}

void MyGLWidget::chooseObj() {
	QString path = QFileDialog::getOpenFileName(this,
		tr("Open File"),
		".",
		tr("Obj Files(*.obj)"));
	if (!path.isEmpty()) {
		readObj(path.toStdString().c_str());
	}
	else {
		QMessageBox::warning(this, tr("Path"),
			tr("You did not select any file."));
	}
}

void MyGLWidget::chooseTexture() {
	QString path = QFileDialog::getOpenFileName(this,
		tr("Open File"),
		".",
		tr("Image Files(*.png)"));
	if (!path.isEmpty()) {
		readTexture(path.toStdString().c_str());
	}
	else {
		QMessageBox::warning(this, tr("Path"),
			tr("You did not select any file."));
	}
}

void MyGLWidget::renderToTexture() {
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	m_frameBuf->bind();
	program.bind(); //glUseProgram  

	program.setUniformValue("tex", 0);
	textureC->bind();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glClearDepth(1.0f);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	drawTriangle();

	QImage fboImage(m_frameBuf->toImage());

	char path[1024];
	sprintf(path, "%s%02d.png", savePath.c_str(), mframeid++);
	fboImage.save(path);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);


	program.release();//glUseProgram(0)  
	m_frameBuf->release();
}

void MyGLWidget::saveImage() {
	renderToTexture();
}

void MyGLWidget::saveImageAs() {
	QString path = QFileDialog::getSaveFileName(this,
		tr("Save File"),
		savePath.c_str(),
		tr("Image Files(*.png)"));
	if (!path.isEmpty()) {
		savePath = path.toStdString();
		savePath = savePath.substr(0, savePath.size() - 4);
		mframeid = 0;
		renderToTexture();
	}
	else {
		QMessageBox::warning(this, tr("Path"),
			tr("You did not select any file."));
	}
}

void MyGLWidget::initializeGL() {
	QGLShader *vshader = new QGLShader(QGLShader::Vertex, this);
	const char *vsrc = ReadShaderFile("shader/default.vert");
	vshader->compileSourceCode(vsrc);

	QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
	const char *fsrc = ReadShaderFile("shader/default.frag");
	fshader->compileSourceCode(fsrc);

	program.addShader(vshader);
	program.addShader(fshader);
	program.link();
	
	mMMatrixHandle = program.uniformLocation("model");
	mVMatrixHandle = program.uniformLocation("view");
	mPMatrixHandle = program.uniformLocation("proj");
	mPositionHandle = program.attributeLocation("attribPosition");
	mTexcoordHandle = program.attributeLocation("attribTexcoord");
    mNormalHandle = program.attributeLocation("attribNormal");
    mTangentHandle = program.attributeLocation("attribTangent");

	//CCW
	/*
	vertices << QVector3D(-1.0f,-1.0f, 0.0f)
			 << QVector3D( 1.0f, 1.0f, 0.0f)
			 << QVector3D(-1.0f, 1.0f, 0.0f)
			 << QVector3D(-1.0f,-1.0f, 0.0f)
			 << QVector3D( 1.0f,-1.0f, 0.0f)
			 << QVector3D( 1.0f, 1.0f, 0.0f);

	vts << QVector2D(0.0f, 0.0f)
		<< QVector2D(1.0f, 1.0f)
		<< QVector2D(0.0f, 1.0f)
		<< QVector2D(0.0f, 0.0f)
		<< QVector2D(1.0f, 0.0f)
		<< QVector2D(1.0f, 1.0f);
	*/
	readObj("mesh/mytt.obj");
	readTexture("mesh/mytt.jpg");

	QOpenGLFramebufferObjectFormat format;
	format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	
	m_frameBuf = new QOpenGLFramebufferObject(G.w, G.h, format);
}

void MyGLWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);

	float ratio = (float)w / h;
	float left = -1.0;
	float right = 1.0;
	float bottom = -1.0f;
	float top = 1.0f;
	float n = 0.999f;
	float f = 10000.0f;

	mProjectionMatrix.frustum(left, right, bottom, top, n, f);
}

void printMatrix(QMatrix4x4 matrix) {
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			std::cout << matrix(i, j) << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void MyGLWidget::drawTriangle() {
	m_vertexBuf.bind();
	program.enableAttributeArray(mPositionHandle);
	//program.setAttributeArray(mPositionHandle, vertices.constData());
	program.setAttributeBuffer(mPositionHandle, GL_FLOAT, 0, 3);

	program.enableAttributeArray(mTexcoordHandle);
	//program.setAttributeArray(mTexcoordHandle, vts.constData());
	program.setAttributeBuffer(mTexcoordHandle, GL_FLOAT, sizeof(GLfloat)*mverts * 3, 2);
	m_vertexBuf.release();

    program.enableAttributeArray(mNormalHandle);
    //program.setAttributeArray(mNormalHandle, vts.constData());
    program.setAttributeBuffer(mNormalHandle, GL_FLOAT, sizeof(GLfloat)*mverts * 5, 3);
    m_vertexBuf.release();
    
    program.enableAttributeArray(mTangentHandle);
    //program.setAttributeArray(mTangentHandle, vts.constData());
    program.setAttributeBuffer(mTangentHandle, GL_FLOAT, sizeof(GLfloat)*mverts * 8, 3);
    m_vertexBuf.release();    

	program.setUniformValue(mMMatrixHandle, mModelMatrix.transposed());
	program.setUniformValue(mVMatrixHandle, mViewMatrix.transposed());
	program.setUniformValue(mPMatrixHandle, mProjectionMatrix.transposed());

	//printMatrix(mModelMatrix);
	//printMatrix(mViewMatrix);
	//printMatrix(mProjectionMatrix);

	//glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	m_indexBuf.bind();
	glDrawElements(GL_TRIANGLES, mfaces*3, GL_UNSIGNED_SHORT, 0);

	program.disableAttributeArray(mPositionHandle);
	program.disableAttributeArray(mTexcoordHandle);
    program.disableAttributeArray(mNormalHandle);
    program.disableAttributeArray(mTangentHandle);
}

void MyGLWidget::setMatrix() {
	///rotate left_buttons
	float n = 0.1, f = 1000;
	float l = -G.cx / G.fx * n, r = (G.w - G.cx) / G.fx * n;
	float b = -G.cy / G.fy * n, t = (G.h - G.cy) / G.fy * n;
	mProjectionMatrix.setToIdentity();
	mProjectionMatrix = {
		2.f * n / (r - l), 0.f, 0.f, 0.f,
		0.f, -2.f * n / (t - b), 0.f, 0.f,
		-(r + l) / (r - l), -(t + b) / (t - b), (f + n) / (f - n), 1.f,
		0.f, 0.f, -(2.f * f*n) / (f - n), 0.f
	};

	mModelMatrix.setToIdentity();
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			mModelMatrix(i,j) = m_arcball.ThisRot.M[i * 3 + j];
		}
	}

	///test
	/*
	CameraPose model_tran(G.Intrinsic, 0., 0., M_PI, 0., 0., 0.);
	float *model_matrix = model_tran.getViewMatrix();
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			mModelMatrix(i, j) = model_matrix[i*4 + j];
		}
	}
	*/

	///scale middle_button
	/*
	QVector3D pos = QVector3D(0.0f, 0.0f, m_arcball.mRadius);
	QVector3D target = QVector3D(0.0f, 0.0f, -5.0f);
	QVector3D up = QVector3D(0.0f, 1.0f, 0.0f);
	//pos = target + (pos - target)*m_arcball.mRadius;
	*/
	mViewMatrix.setToIdentity();
	Eigen::Matrix3d poseR = Eigen::AngleAxisd(M_PI, Eigen::Vector3d(1.0, 0.0, 0.0).normalized()).toRotationMatrix();
	//Eigen::Matrix3d poseR = Eigen::AngleAxisd(M_PI, Eigen::Vector3d(0.0, 0.0, 1.0).normalized()).toRotationMatrix();
	//poseR = Eigen::Matrix3d::Identity();
	Eigen::Vector3d poset = Eigen::Vector3d(m_arcball.translate_x / 10.0, -m_arcball.translate_y / 10.0, 5 + m_arcball.mRadius);
	pose = mf::CameraPose(G.Intrinsic, poseR, poset);
	/*
	poseR = Eigen::AngleAxisd(0.3, Eigen::Vector3d(0.0, 1.0, 0.0).normalized()).toRotationMatrix();
	poset = Eigen::Vector3d(0.0, 0.2, 0.0);
	CameraPose p21(G.Intrinsic, poseR, poset);
	Sophus::SE3d SE3 = p21.SE3_Rt * pose.SE3_Rt;
	pose = CameraPose(G.Intrinsic, SE3);
	*/
	float *view = pose.getViewMatrix();
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			mViewMatrix(i, j) = view[i * 4 + j];
 		}
	}
	/*
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j)
			std::cout << mViewMatrix(i, j) << " ";
		std::cout << std::endl;
	}
	*/
	//mViewMatrix.lookAt(pos, target, up);

	///translate right_button
	//mModelMatrix.translate(m_arcball.translate_x / 10.0, m_arcball.translate_y / 10.0, 0.0f);
	//mViewMatrix.translate(m_arcball.translate_x / 10.0, m_arcball.translate_y / 10.0, 2.0 + m_arcball.mRadius);
	/*
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			pose.R(i, j) = mMVPMatrix(j, i);
		}
	}
	*/

	//pose.R = pose.R.inverse();
	//pose.rotation = pose.getRotation(pose.rot);
	//pose.rotation.print("rotation");
	//pose.rotation[2] *= -1;
	//pose.rotation[0] *= -1;
	//pose.rotation[1] *= -1;
	//pose.setMatrix();
	//pose.rot = pose.rot.inv();

	//pose.t[0] = -m_arcball.translate_x / 10;
	//pose.t[1] = -m_arcball.translate_y / 10;
	//pose.t[2] = 2.0 + m_arcball.mRadius;

	//pose.refreshByARt();
	//Matrix33d A(700, 0.0, 313, 0.0, 700.0, 256.0, 0.0, 0.0, 1.0);
	//pose = CameraPose(A, 0, 0, 1, 0.0, 0, 5);

	/*
	pose.rot.print("rot");
	pose.translation.print("trans");
	pose.rotation = pose.getRotation(pose.rot);
	pose.rotation.print("rot");
	
	pose.q.print("q");
	*/
	//program.setUniformValue("Q", iQ);
	//program.setUniformValue("Axis", GLint(axis_tmp));
	//program.setUniformValue("flag", GLint(0));
	//program.setUniformValue("size", GLfloat(4.0));
	//program.setUniformValue("Mu", GLint(8));
	//program.setUniformValue(mqHandle, q);

}

void MyGLWidget::paintGL()
{
	QPainter painter;
	painter.begin(this);

	painter.beginNativePainting();
	glClearColor(0.3f, 0.3f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	program.bind(); //glUseProgram  
	setMatrix();

	program.setUniformValue("tex", 0);
	textureC->bind();
	//program.setUniformValue("modelSW", 1);
	//textureSW->bind(1);



	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glClearDepth(1.0f);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	drawTriangle();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);


	program.release();//glUseProgram(0)  
	/*
	painter.endNativePainting();


	// Do a complete rotation every 10 seconds.  
	long t = time.elapsed() % 10000L;
	float angleInDegrees = (360.0f / 10000.0f) * ((int)t);

	QString framesPerSecond;
	framesPerSecond.setNum(frames / (time.elapsed() / 1000.0), 'f', 2);

	QString mRadiusData;
	mRadiusData.setNum(m_arcball.mRadius);

	painter.setPen(Qt::white);

	//painter.drawText(20, 40, framesPerSecond + " fps");
	painter.drawText(20, 20, "mRadius: " + mRadiusData);

	painter.end();
	*/
	if (!(frames % 1000)) {
		time.start();
		frames = 0;
	}
	
	swapBuffers();
	frames++;
}

void MyGLWidget::mousePressEvent(QMouseEvent*mouse_event)
{
	QPoint qpoint = mapFromGlobal(QCursor::pos());
	int posx = qpoint.x();
	int posy = qpoint.y();
	if (mouse_event->button() == Qt::LeftButton)
	{
		m_arcball.MousePt.s.X = posx;
		m_arcball.MousePt.s.Y = posy;

		m_arcball.LastRot = m_arcball.ThisRot;
		m_arcball.ArcBall.click(&m_arcball.MousePt);
		m_arcball.button_status = 1;
	}
	else if (mouse_event->button() == Qt::RightButton)
		m_arcball.button_status = 2;
	m_arcball.mLastMousePos.s.X = static_cast<LONG>(qpoint.x());
	m_arcball.mLastMousePos.s.Y = static_cast<LONG>(qpoint.y());
	setMouseTracking(true);
	grabMouse();
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *mouse_event)
{
	QPoint qpoint = mapFromGlobal(QCursor::pos());
	int posx = qpoint.x();
	int posy = qpoint.y();
	if (m_arcball.button_status == 1)
	{
		m_arcball.MousePt.s.X = posx;
		m_arcball.MousePt.s.Y = posy;
		Quat4fT     ThisQuat;
		m_arcball.ArcBall.drag(&m_arcball.MousePt, &ThisQuat);
		Matrix3fSetRotationFromQuat4f(&m_arcball.ThisRot, &ThisQuat);
		Matrix3fMulMatrix3f(&m_arcball.ThisRot, &m_arcball.LastRot);
		Matrix4fSetRotationFromMatrix3f(&m_arcball.Transform, &m_arcball.ThisRot);
	}
	else if (m_arcball.button_status == 2)
	{
		float dx = 0.1f*static_cast<float>(qpoint.x() - m_arcball.mLastMousePos.s.X);
		float dy = 0.1f*static_cast<float>(qpoint.y() - m_arcball.mLastMousePos.s.Y);
		m_arcball.translate_x += dx;
		m_arcball.translate_y -= dy;
	}
	m_arcball.mLastMousePos.s.X = posx;
	m_arcball.mLastMousePos.s.Y = posy;
}

void MyGLWidget::mouseReleaseEvent(QMouseEvent *mouse_event)
{
	releaseMouse();
	setMouseTracking(false);
}

void MyGLWidget::wheelEvent(QWheelEvent *event)
{
	float delta = (float)event->delta() / 50.0f;
	m_arcball.mRadius *= (1 - delta);
}