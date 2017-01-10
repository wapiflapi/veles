/*
 * Copyright 2016 CodiLime, Copyright 2017 Wannes `wapiflapi` Rombouts
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
#include "visualisation/ngram.h"
#include "util/settings/shortcutmanager.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <vector>

#include <QDebug>
#include <QtMath>
#include <QTime>

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QSlider>
#include <QPixmap>
#include <QBitmap>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QButtonGroup>
#include <QShortcut>
#include <QTimer>

namespace veles {
namespace visualisation {

const int k_minimum_brightness = 25;
const int k_maximum_brightness = 103;
const double k_brightness_heuristic_threshold = 0.66;
const int k_brightness_heuristic_min = 38;
const int k_brightness_heuristic_max = 66;
// decrease this to reduce noise (but you may lose data if you overdo it)
const double k_brightness_heuristic_scaling = 2.0;

NGramWidget::NGramWidget(QWidget *parent) :
  VisualisationWidget(parent),
  c_sph(0), c_cyl(0),
  c_flat(0), c_layered_x(0), c_layered_z(0),
  cam_targeting(false),
  mode_flat_(false), mode_layered_x_(false), mode_layered_z_(false),
  shape_(EVisualisationShape::CUBE),
  brightness_((k_maximum_brightness + k_minimum_brightness) / 2),
  rotationAxis(QVector3D(-1, 1, 0).normalized()), angularSpeed(0.3),
  position(0, 0, -5), movement(0, 0, 0), speed(0, 0, 0),
  brightness_slider_(nullptr), is_playing_(true),
  use_brightness_heuristic_(true) {

  setFocusPolicy(Qt::StrongFocus);
  QTimer::singleShot(0, this, SLOT(setFocus()));
}



NGramWidget::~NGramWidget() {
  makeCurrent();
  delete texture;
  delete databuf;
  doneCurrent();
}

void NGramWidget::setBrightness(const int value) {
  brightness_ = value;
  c_brightness = static_cast<float>(value) * value * value;
  c_brightness /= getDataSize();
  c_brightness = std::min(1.2f, c_brightness);
}

void NGramWidget::refresh() {
  if (use_brightness_heuristic_) {
    autoSetBrightness();
  }
  setBrightness(brightness_);
  makeCurrent();
  delete texture;
  delete databuf;
  initTextures();
  doneCurrent();
}

bool NGramWidget::prepareOptionsPanel(QBoxLayout *layout) {
  VisualisationWidget::prepareOptionsPanel(layout);

  QLabel *brightness_label = new QLabel("Brightness: ");
  brightness_label->setAlignment(Qt::AlignTop);
  layout->addWidget(brightness_label);

  brightness_ = suggestBrightness();
  brightness_slider_ = new QSlider(Qt::Horizontal);
  brightness_slider_->setMinimum(k_minimum_brightness);
  brightness_slider_->setMaximum(k_maximum_brightness);
  brightness_slider_->setValue(brightness_);
  connect(brightness_slider_, &QSlider::valueChanged, this,
          &NGramWidget::brightnessSliderMoved);
  layout->addWidget(brightness_slider_);

  use_heuristic_checkbox_ = new QCheckBox(
    "Automatically adjust brightness");
  use_heuristic_checkbox_->setChecked(use_brightness_heuristic_);
  connect(use_heuristic_checkbox_, &QCheckBox::stateChanged,
          this, &NGramWidget::setUseBrightnessHeuristic);
  layout->addWidget(use_heuristic_checkbox_);

  pause_button_ = new QPushButton();
  pause_button_->setIcon(getColoredIcon(":/images/pause.png"));
  layout->addWidget(pause_button_);
  connect(pause_button_, &QPushButton::released,
	  this, &NGramWidget::playPause);
  veles::util::settings::shortcutManager
    ->managedShortcut("playpause", "toggle animation", "Space",
		      pause_button_, &QPushButton::click);


  QHBoxLayout *mode_buttons = new QHBoxLayout();

  mode_flat_pushbutton_ = new QPushButton();
  mode_flat_pushbutton_->setCheckable(true);
  mode_flat_pushbutton_->setIcon(getColoredIcon(":/images/flat.png", false));
  mode_flat_pushbutton_->setIconSize(QSize(32, 32));
  connect(mode_flat_pushbutton_, &QPushButton::toggled,
	  this, &NGramWidget::setFlat);
  veles::util::settings::shortcutManager
    ->managedShortcut("flatmode", "toggle flat mode", "4",
		      mode_flat_pushbutton_, &QPushButton::click);

  mode_layered_x_pushbutton_ = new QPushButton();
  mode_layered_x_pushbutton_->setCheckable(true);
  mode_layered_x_pushbutton_->setIcon(getColoredIcon(":/images/sorted.png"));
  mode_layered_x_pushbutton_->setIconSize(QSize(32, 32));
  connect(mode_layered_x_pushbutton_, &QPushButton::toggled,
	  this, &NGramWidget::setLayeredX);
  veles::util::settings::shortcutManager
    ->managedShortcut("sorted", "toggle sorted mode", "5",
		      mode_layered_x_pushbutton_, &QPushButton::click);

  mode_layered_z_pushbutton_ = new QPushButton();
  mode_layered_z_pushbutton_->setCheckable(true);
  mode_layered_z_pushbutton_->setIcon(getColoredIcon(":/images/layered.png"));
  mode_layered_z_pushbutton_->setIconSize(QSize(32, 32));
  connect(mode_layered_z_pushbutton_, &QPushButton::toggled,
	  this, &NGramWidget::setLayeredZ);
  veles::util::settings::shortcutManager
    ->managedShortcut("layeredmode", "toggle layered mode", "6",
		      mode_layered_z_pushbutton_, &QPushButton::click);


  mode_buttons->addWidget(mode_flat_pushbutton_);
  mode_buttons->addWidget(mode_layered_x_pushbutton_);
  mode_buttons->addWidget(mode_layered_z_pushbutton_);
  layout->addLayout(mode_buttons);

  QHBoxLayout *shape_buttons = new QHBoxLayout();
  QButtonGroup *shape_button_group = new QButtonGroup();

  cube_button_ = new QPushButton();
  cube_button_->setCheckable(true);
  cube_button_->setChecked(true);
  cube_button_->setIcon(getColoredIcon(":/images/cube.png", false));
  cube_button_->setIconSize(QSize(32, 32));
  connect(cube_button_, &QPushButton::released,
          std::bind(&NGramWidget::setShape, this,
                    EVisualisationShape::CUBE));
  veles::util::settings::shortcutManager
    ->managedShortcut("cubeshape", "switch to cube shape", "1",
		      cube_button_, &QPushButton::click);

  cylinder_button_ = new QPushButton();
  cylinder_button_->setCheckable(true);
  cylinder_button_->setIcon(getColoredIcon(":/images/cylinder.png", false));
  cylinder_button_->setIconSize(QSize(32, 32));
  connect(cylinder_button_, &QPushButton::released,
          std::bind(&NGramWidget::setShape, this,
                    EVisualisationShape::CYLINDER));
  veles::util::settings::shortcutManager
    ->managedShortcut("cylindershape", "switch to cylinder shape", "2",
		      cylinder_button_, &QPushButton::click);

  sphere_button_ = new QPushButton();
  sphere_button_->setCheckable(true);
  sphere_button_->setIcon(getColoredIcon(":/images/sphere.png"));
  sphere_button_->setIconSize(QSize(32, 32));
  connect(sphere_button_, &QPushButton::released,
          std::bind(&NGramWidget::setShape, this,
                    EVisualisationShape::SPHERE));
  veles::util::settings::shortcutManager
    ->managedShortcut("sphereshape", "switch to sphere shape", "3",
		      sphere_button_, &QPushButton::click);

  shape_buttons->addWidget(cube_button_);
  shape_buttons->addWidget(cylinder_button_);
  shape_buttons->addWidget(sphere_button_);

  shape_button_group->addButton(cube_button_);
  shape_button_group->addButton(cylinder_button_);
  shape_button_group->addButton(sphere_button_);
  shape_button_group->setExclusive(true);

  layout->addLayout(shape_buttons);

  center_button_ = new QPushButton("center view");
  layout->addWidget(center_button_);
  connect(center_button_, &QPushButton::released, this, &NGramWidget::centerView);
  veles::util::settings::shortcutManager
    ->managedShortcut("centerview", "center view", "0",
		      center_button_, &QPushButton::click);


  return true;
}

int NGramWidget::suggestBrightness() {
  int size = getDataSize();
  auto data = reinterpret_cast<const uint8_t*>(getData());
  if (size < 100) {
    return (k_minimum_brightness + k_maximum_brightness) / 2;
  }
  std::vector<uint64_t> counts(256, 0);
  for (int i = 0; i < size; ++i) {
    counts[data[i]] += 1;
  }
  std::sort(counts.begin(), counts.end());
  int offset = 0, sum = 0;
  while (offset < 255 && sum < k_brightness_heuristic_threshold * size) {
    sum += counts[255 - offset];
    offset += 1;
  }
  offset = static_cast<int>(static_cast<double>(offset)
                            / k_brightness_heuristic_scaling);
  return std::max(k_brightness_heuristic_min,
                  k_brightness_heuristic_max - offset);
}

void NGramWidget::playPause() {
  QPixmap pixmap;
  if (is_playing_) {
    pause_button_->setIcon(getColoredIcon(":/images/play.png"));
  } else {
    pause_button_->setIcon(getColoredIcon(":/images/pause.png"));
  }
  is_playing_ = !is_playing_;
  if (is_playing_ && angularSpeed == 0) {
    angularSpeed = 0.3;
  }
}

void NGramWidget::setFlat(bool val) {
  mode_layered_z_pushbutton_->setEnabled(!val);
  mode_flat_ = val;
}

void NGramWidget::setLayeredX(bool val) {
  if (!QGuiApplication::keyboardModifiers() &&
      val && mode_layered_z_pushbutton_->isChecked()) {
    mode_layered_z_pushbutton_->setChecked(false);
  }
  mode_layered_x_ = val;
}

void NGramWidget::setLayeredZ(bool val) {
  if (!QGuiApplication::keyboardModifiers() &&
      val && mode_layered_x_pushbutton_->isChecked()) {
    mode_layered_x_pushbutton_->setChecked(false);
  }
  mode_layered_z_ = val;
}


void NGramWidget::setShape(EVisualisationShape shape) {
  shape_ = shape;
}

void NGramWidget::brightnessSliderMoved(int value) {
  if (value == brightness_) return;
  use_brightness_heuristic_ = false;
  use_heuristic_checkbox_->setChecked(false);
  setBrightness(value);
}

void NGramWidget::setUseBrightnessHeuristic(int state) {
  use_brightness_heuristic_ = state;
  if (use_brightness_heuristic_) {
    autoSetBrightness();
  }
}

void NGramWidget::autoSetBrightness() {
  auto new_brightness = suggestBrightness();
  if (new_brightness == brightness_) return;
  brightness_ = new_brightness;
  if (brightness_slider_ != nullptr) {
    brightness_slider_->setValue(brightness_);
  }
  setBrightness(brightness_);
}

void NGramWidget::timerEvent(QTimerEvent *e) {

  // neat trick here to transform towards projections,
  // always move towards it, fix later if we went to far.

  if (shape_ == EVisualisationShape::CYLINDER) {
    c_cyl += 0.01;
  } else {
    c_cyl -= 0.01;
  }

  if (shape_ == EVisualisationShape::SPHERE) {
    c_sph += 0.01;
  } else {
    c_sph -= 0.01;
  }

  if (mode_flat_) {
    c_flat += 0.01;
  } else {
    c_flat -= 0.01;
  }

  if (mode_layered_x_) {
    c_layered_x += 0.01;
  } else {
    c_layered_x -= 0.01;
  }

  if (mode_layered_z_) {
    c_layered_z += 0.01;
  } else {
    c_layered_z -= 0.01;
  }

  if (c_cyl > 1) c_cyl = 1;
  if (c_cyl < 0) c_cyl = 0;
  if (c_sph > 1) c_sph = 1;
  if (c_sph < 0) c_sph = 0;

  if (c_flat > 1) c_flat = 1;
  if (c_flat< 0) c_flat = 0;

  if (c_layered_x > 1) c_layered_x = 1;
  if (c_layered_x < 0) c_layered_x = 0;

  if (c_layered_z > 1) c_layered_z = 1;
  if (c_layered_z < 0) c_layered_z = 0;


  // Rotation

  if (!is_playing_) {
    // Decrease angular speed (friction)
    angularSpeed *= 0.90;
  }

  if (angularSpeed < 0.01) {
    angularSpeed = 0.0; // Stop rotation when speed goes below threshold
  } else {
    rotation = QQuaternion::fromAxisAndAngle(rotationAxis, angularSpeed) * rotation;
  }

  if (cam_target_rot) {
    // Target the camera rotation.
    QQuaternion target_rot = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 0);

    if (rotation != target_rot) {
      rotation = 0.9 * rotation + 0.1 * target_rot;
    }
    if ((target_rot - rotation).length() < 0.01) {
      cam_target_rot = false;
      rotation = target_rot;
    }
  }

  // movement speed

  float max_speed = 5.0;
  if (speed.length() < max_speed) {
    float cameramod = qLn(qSqrt(1 + position.length()));
    speed += cameramod * movement + 0.2 * speed.length() * movement;
  }

  if (speed.x() > -0.001 && 0.001 > speed.x()) {
    speed.setX(0);
  }

  if (speed.y() > -0.001 && 0.001 > speed.y()) {
    speed.setY(0);
  }

  if (speed.z() > -0.001 && 0.001 > speed.z()) {
    speed.setZ(0);
  }


  // If targeting the camera overwrite the speed.
  QVector3D target_delta = cam_target - position;
  if (cam_targeting) {
    QVector3D mov = (cam_target - position).normalized();
    speed = qPow(1.0 + target_delta.length(), 2.0) * mov;
  }

  position += speed * 0.01;
  speed *= 0.90;


  if (cam_targeting) {

    // If we arrived, OR we went too far (overshoot), fix position and stop.
    if ((cam_target - position).normalized() != target_delta.normalized() ||
	cam_target == position) {
      cam_targeting = false;
      position = cam_target;
      speed *= 0;
    }
  }

  // Request an update
  // TODO: only if something is changing.
  update();
}

void NGramWidget::initializeVisualisationGL() {
  initializeOpenGLFunctions();

  setMouseTracking(true);


  glClearColor(0, 0, 0, 1);

  autoSetBrightness();

  initShaders();
  initTextures();
  initGeometry();
  setBrightness(brightness_);
}

void NGramWidget::initShaders() {
  if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                       ":/ngram/vshader.glsl"))
    close();

  // Compile fragment shader
  if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                       ":/ngram/fshader.glsl"))
    close();

  // Link shader pipeline
  if (!program.link()) close();

  timer.start(16, this);
}

void NGramWidget::initTextures() {
  int size = getDataSize();
  const uint8_t *data = reinterpret_cast<const uint8_t*>(getData());

  databuf = new QOpenGLBuffer(QOpenGLBuffer::Type(GL_TEXTURE_BUFFER));
  databuf->create();
  databuf->bind();
  databuf->allocate(data, size);
  databuf->release();

  texture = new QOpenGLTexture(QOpenGLTexture::TargetBuffer);
  texture->setSize(size);
  texture->setFormat(QOpenGLTexture::R8U);
  texture->create();
  texture->bind();
  glTexBuffer(GL_TEXTURE_BUFFER, QOpenGLTexture::R8U, databuf->bufferId());
}

void NGramWidget::initGeometry()
{
  vao.create();
}

void NGramWidget::resizeGL(int w, int h)
{
  width = w;
  height = h;

  // Re-init our perspective.

  perspective.setToIdentity();
  if (width > height) {
    perspective.perspective(45, static_cast<double>(width) / height, 0.001f, 100.0f);
  } else {
    // wtf h4x.  gluPerspective fixes the viewport wrt y field of view, and
    // we need to fix it to x instead.  So, rotate the world, fix to new y,
    // rotate it back.
    perspective.rotate(90, 0, 0, 1);
    perspective.perspective(45, static_cast<double>(height) / width, 0.001f, 100.0f);
    perspective.rotate(-90, 0, 0, 1);
  }

}

void NGramWidget::keyPressEvent(QKeyEvent *event)
{

  if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
    movement.setX(1);
  }

  if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
    movement.setX(-1);
  }

  if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S) {
    if (event->modifiers() & Qt::ShiftModifier) {
      movement.setZ(-1);
    } else {
      movement.setY(1);
    }
  }

  if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W) {
    if (event->modifiers() & Qt::ShiftModifier) {
      movement.setZ(1);
    } else {
      movement.setY(-1);
    }
  }

  if (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_Q) {
    movement.setZ(-1);
  }

  if (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_E) {
    movement.setZ(1);
  }

}

void NGramWidget::keyReleaseEvent(QKeyEvent *event)
{

  if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
      event->key() == Qt::Key_A || event->key() == Qt::Key_D) {
    movement.setX(0);
  }

  if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up ||
      event->key() == Qt::Key_S || event->key() == Qt::Key_W) {
    if (event->modifiers() & Qt::ShiftModifier) {
      movement.setZ(0);
    } else {
      movement.setY(0);
    }
  }

  if (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_PageUp ||
      event->key() == Qt::Key_Q || event->key() == Qt::Key_E) {
    movement.setZ(0);
  }


}

void NGramWidget::centerView() {
  cam_targeting = true;

  // We want flat projection by default in two cases.
  if (mode_flat_ && shape_ != EVisualisationShape::SPHERE) {
    if (is_playing_) {
      playPause();
    }
    cam_target = QVector3D(0, 0, -2.414);
    cam_target_rot = true;
  } else {
    if (!is_playing_) {
      playPause();
    }
    cam_target = QVector3D(0, 0, -5);
    cam_target_rot = false;
  }
}

void NGramWidget::mousePressEvent(QMouseEvent *event)
{
  // Save mouse press position
  mousePressPosition = QVector2D(event->localPos());

  angularSpeed = 0;
}

void NGramWidget::mouseMoveEvent(QMouseEvent *event)
{

  if (!(event->buttons() & Qt::LeftButton)) {
    return;
  }

  // Mouse release position - mouse press position
  QVector2D diff = QVector2D(event->localPos()) - mousePressPosition;

  // Rotation axis is perpendicular to the mouse position difference
  // vector
  QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();

  // Accelerate angular speed relative to the length of the mouse sweep
  qreal acc = (diff.length() * diff.length()) / qSqrt(width * width + height * height);


  if (acc != 0) {
    rotationAxis = (0.25 * angularSpeed * 0.75 * rotationAxis + n * acc).normalized();
    angularSpeed += acc;
    mousePressPosition = QVector2D(event->localPos());
  }



}

void NGramWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // Mouse release position - mouse press position
    QVector2D diff = QVector2D(event->localPos()) - mousePressPosition;

    // Rotation axis is perpendicular to the mouse position difference
    // vector
    QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();

    // Accelerate angular speed relative to the length of the mouse sweep
    qreal acc = (diff.length() * diff.length()) / qSqrt(width * width + height * height);


    if (acc != 0) {
      rotationAxis = (0.25 * angularSpeed * 0.75 * rotationAxis + n * acc).normalized();
      angularSpeed += acc;
    }


     if (is_playing_ ^ bool(event->modifiers() & Qt::ShiftModifier)) {
      playPause();
    }

    // This is satisfying.
    if (!is_playing_ && angularSpeed > 5) {
      angularSpeed = angularSpeed / 15;
      playPause();
    }
}

void NGramWidget::wheelEvent(QWheelEvent *event) {

  float movement = (event->delta() / 8) / 15;
  float zoom = 2 * movement * qAbs(movement);
  speed.setZ(speed.z() + zoom);
}

void NGramWidget::paintGL() {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

  unsigned size = getDataSize();

  program.bind();
  texture->bind();
  vao.bind();

  int loc_sz = program.uniformLocation("sz");
  program.setUniformValue("tx", 0);
  program.setUniformValue("c_cyl", c_cyl);
  program.setUniformValue("c_sph", c_sph);

  program.setUniformValue("c_flat", c_flat);
  program.setUniformValue("c_layered_x", c_layered_x);
  program.setUniformValue("c_layered_z", c_layered_z);
  program.setUniformValue("c_brightness", c_brightness);

  // Calculate model view transformation
  QMatrix4x4 matrix;

  matrix.translate(position);
  matrix.rotate(rotation);

  // projection matrices.
  program.setUniformValue("matrix", matrix);

  program.setUniformValue("perspective", perspective);

  // testomg
  float sz = std::min(width, height) / 256.0;
  program.setUniformValue("voxsz", sz);

  glEnable(GL_PROGRAM_POINT_SIZE);

  glUniform1ui(loc_sz, size);

  glDrawArrays(GL_POINTS, 0, size - 2);
}

}  // namespace visualisation
}  // namespace veles
