/*
 * Copyright 2016 CodiLime
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "visualisation/digram.h"


#include <algorithm>
#include <functional>
#include <vector>

#include <QPixmap>
#include <QBitmap>



namespace veles {
namespace visualisation {

DigramWidget::DigramWidget(QWidget *parent) :
  VisualisationWidget(parent),
  c_sqr(0), c_cir(0),
  shape_(EVisualisationShape::SQUARE), mode_(EVisualisationMode::BIGRAM) {}

DigramWidget::~DigramWidget() {
  makeCurrent();
  delete texture_;
  square_vertex_.destroy();
  doneCurrent();
}

void DigramWidget::refresh() {
  makeCurrent();
  delete texture_;
  initTextures();
  doneCurrent();
  update();
}

bool DigramWidget::prepareOptionsPanel(QBoxLayout *layout) {
  VisualisationWidget::prepareOptionsPanel(layout);


  QHBoxLayout *shape_box = new QHBoxLayout();
  square_button_ = new QPushButton();
  square_button_->setIcon(getColoredIcon(":/images/cube.png", false));
  square_button_->setIconSize(QSize(32, 32));
  connect(square_button_, &QPushButton::released,
          std::bind(&DigramWidget::setShape, this,
                    EVisualisationShape::SQUARE));
  shape_box->addWidget(square_button_);

  circle_button_ = new QPushButton();
  circle_button_->setIcon(getColoredIcon(":/images/sphere.png"));
  circle_button_->setIconSize(QSize(32, 32));

  connect(circle_button_, &QPushButton::released,
          std::bind(&DigramWidget::setShape, this,
                    EVisualisationShape::CIRCLE));
  shape_box->addWidget(circle_button_);


  layout->addLayout(shape_box);

  return true;
}


void DigramWidget::setShape(EVisualisationShape shape) {
  shape_ = shape;
}



void DigramWidget::timerEvent(QTimerEvent *e) {

  // neat trick here to transform towards projections,
  // always move towards it, fix later if we went to far.

  if (shape_ == EVisualisationShape::SQUARE) {
    c_sqr += 0.01;
  } else {
    c_sqr -= 0.01;
  }

  if (shape_ == EVisualisationShape::CIRCLE) {
    c_cir += 0.01;
  } else {
    c_cir -= 0.01;
  }

  if (c_sqr > 1) c_sqr = 1;
  if (c_sqr < 0) c_sqr = 0;
  if (c_cir > 1) c_cir = 1;
  if (c_cir < 0) c_cir = 0;

  // Request an update
  // TODO: only if something is changing.
  update();
}



void DigramWidget::initializeVisualisationGL() {
  initializeOpenGLFunctions();

  glClearColor(0, 0, 0, 1);

  initShaders();
  initTextures();
  initGeometry();
}

void DigramWidget::initShaders() {
  if (!program_.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                       ":/digram/vshader.glsl"))
    close();

  // Compile fragment shader
  if (!program_.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                       ":/digram/fshader.glsl"))
    close();

  // Link shader pipeline
  if (!program_.link()) close();

  // Bind shader pipeline for use
  if (!program_.bind()) close();

  timer.start(16, this);
}

void DigramWidget::initTextures() {
  texture_ = new QOpenGLTexture(QOpenGLTexture::Target2D);
  texture_->setSize(256, 256);
  texture_->setFormat(QOpenGLTexture::RG32F);
  texture_->allocateStorage();

  // effectively arrays of size [256][256][2], represented as single blocks
  auto bigtab = new uint64_t[256 * 256 * 2];
  memset(bigtab, 0, 256 * 256 * 2 * sizeof(*bigtab));
  auto ftab = new float[256 * 256 * 2];
  const uint8_t *rowdata = reinterpret_cast<const uint8_t *>(getData());

  // Build a histogram
  for (size_t i = 0; i < getDataSize() - 1; i++) { // FIXME off by one?
    size_t index = (rowdata[i] * 256 + rowdata[i + 1]) * 2;
    bigtab[index]++;		// count hits
    bigtab[index + 1] += i;	// keeps track of average source position.
  }

  // wapiflapi: we can do a cumulative projection here.
  // basicaly we sort the histogram and assign color based on rank not value.
  // the problem is sortigng is slow, can we fix that?

  // Convert histogram to percentages (realy 0 < floats < 1.0)
  for (int i = 0; i < 256; i++) {
    for (int j = 0; j < 256; j++) {
      size_t index = (i * 256 + j) * 2;
      ftab[index] = static_cast<float>(bigtab[index]) / getDataSize();
      ftab[index+1] =
	static_cast<float>(bigtab[index+1]) / getDataSize() / getDataSize();
    }
  }

  texture_->setData(QOpenGLTexture::RG, QOpenGLTexture::Float32,
                   reinterpret_cast<void *>(ftab));
  texture_->generateMipMaps();

  texture_->setMinificationFilter(QOpenGLTexture::Nearest);

  texture_->setMagnificationFilter(QOpenGLTexture::Linear);

  texture_->setWrapMode(QOpenGLTexture::ClampToEdge);

  delete[] bigtab;
  delete[] ftab;
}

void DigramWidget::initGeometry() {
  square_vertex_.create();
  QVector2D v[] = {
      {0, 0}, {0, 1}, {1, 0}, {1, 1},
  };
  square_vertex_.bind();
  square_vertex_.allocate(v, sizeof v);

  vao_.create();
}

void DigramWidget::resizeGL(int w, int h) {}

void DigramWidget::paintGL() {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  texture_->bind();
  vao_.bind();

  program_.setUniformValue("c_sqr", c_sqr);
  program_.setUniformValue("c_cir", c_cir);

  square_vertex_.bind();
  int vertexLocation = program_.attributeLocation("a_position");
  program_.enableAttributeArray(vertexLocation);
  program_.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 2,
                              sizeof(QVector2D));
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  program_.setUniformValue("tx", 0);
}

}  // namespace visualisation
}  // namespace veles
