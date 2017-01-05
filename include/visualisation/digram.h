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
#ifndef DIGRAM_H
#define DIGRAM_H

#include "visualisation/digram.h"

#include <stdint.h>

#include <vector>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions_3_2_Core>
#include <QBasicTimer>
#include <QPushButton>

#include "visualisation/base.h"

namespace veles {
namespace visualisation {

class DigramWidget : public VisualisationWidget {
  Q_OBJECT

 public:

  enum class EVisualisationShape {SQUARE, CIRCLE};
  enum class EVisualisationMode {BIGRAM};

  explicit DigramWidget(QWidget *parent = 0);
  ~DigramWidget();


 protected:
  void refresh() override;
  bool prepareOptionsPanel(QBoxLayout *);
  void setShape(EVisualisationShape);
  void timerEvent(QTimerEvent *);

  void initializeVisualisationGL() override;

  void resizeGL(int w, int h) override;
  void paintGL() override;

  void initShaders();
  void initTextures();
  void initGeometry();

 private:

  QBasicTimer timer;
  QOpenGLShaderProgram program_;
  QOpenGLTexture *texture_;

  QOpenGLBuffer square_vertex_;
  QOpenGLVertexArrayObject vao_;
  float c_sqr, c_cir;


  EVisualisationShape shape_;
  EVisualisationMode mode_;

  QPushButton *square_button_, *circle_button_;

};

}  // namespace visualisation
}  // namespace veles

#endif
