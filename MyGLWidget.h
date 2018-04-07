#pragma once

#include <QGLWidget>
#include <QGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QVector>
#include <QTime>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFileDialog>
#include <QMessageBox>

#include "MyArcball.h"
#include "utils.h"

class MyGLWidget : public QGLWidget
{
	Q_OBJECT

public:
	MyGLWidget(QWidget *parent);
	~MyGLWidget();

public slots:
	void chooseObj();
	void chooseTexture();
	void saveImage();
	void saveImageAs();

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);

	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent *);
	virtual void mouseReleaseEvent(QMouseEvent *);
	virtual void wheelEvent(QWheelEvent *event);

private:
	const char* ReadShaderFile(const char* filepath);
	void drawTriangle();
	void setMatrix();
	bool readObj(const char *path);
	bool readTexture(const char *path);
	void renderToTexture();

	QGLShaderProgram program;
	QOpenGLTexture *textureC;
	QOpenGLBuffer m_vertexBuf;
	QOpenGLBuffer m_indexBuf;
	QOpenGLFramebufferObject *m_frameBuf;

	mf::CameraPose pose;

	QVector<QVector3D> vertices;
	QVector<QVector2D> vts;
	GLushort *indices = nullptr;
	GLfloat *verts = nullptr;

	//QVector<QVector3D> faces;
	int mMMatrixHandle;
	int mVMatrixHandle;
	int mPMatrixHandle;
	int mPositionHandle;
	int mTexcoordHandle;
    int mNormalHandle;
    int mTangentHandle;
	int mAxisHandle;
	int mInvQHandle;
	int mqHandle;
	int mfaces;
	int mverts;
	int mframeid;

	std::string savePath;
	
	QMatrix4x4 mModelMatrix;//ģ�;���
	QMatrix4x4 mViewMatrix; //��ͼ����  
	QMatrix4x4 mProjectionMatrix; //ͶӰ����  
	QMatrix4x4 mMVPMatrix; //MVP����

	MyArcball m_arcball;
	int frames;
	QTime time;
	QTimer *timer;
};
