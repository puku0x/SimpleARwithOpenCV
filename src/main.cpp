// OpenCV
#include <opencv2/opencv.hpp>

// OpenGL
#include <GL/glut.h>

// マーカ検出
#include ".\SimpleAR\MarkerDetector.hpp"

// チェスボードのパラメータ
#define PAT_ROW    (7)                  // パターンの行数
#define PAT_COL    (10)                 // パターンの列数
#define CHESS_SIZE (24.0)               // パターン一つの大きさ [mm]

// グローバル変数
cv::VideoCapture cap;
CameraCalibration calibration;

// --------------------------------------------------------------------------
// buildProjectionMatrix(カメラ行列, 画面幅, 画面高さ, 出力される)
// 画面の更新時に呼ばれる関数です
// 戻り値: カメラ行列から計算した透視投影行列
// --------------------------------------------------------------------------
Matrix44 buildProjectionMatrix(Matrix33 cameraMatrix, int screen_width, int screen_height)
{
    float near = 0.01;  // Near clipping distance
    float far  = 100;   // Far clipping distance
    
    // Camera parameters
    float f_x = cameraMatrix.data[0]; // Focal length in x axis
    float f_y = cameraMatrix.data[4]; // Focal length in y axis (usually the same?)
    float c_x = cameraMatrix.data[2]; // Camera primary point x
    float c_y = cameraMatrix.data[5]; // Camera primary point y

    Matrix44 projectionMatrix;
    projectionMatrix.data[0] = - 2.0 * f_x / screen_width;
    projectionMatrix.data[1] = 0.0;
    projectionMatrix.data[2] = 0.0;
    projectionMatrix.data[3] = 0.0;
    
    projectionMatrix.data[4] = 0.0;
    projectionMatrix.data[5] = 2.0 * f_y / screen_height;
    projectionMatrix.data[6] = 0.0;
    projectionMatrix.data[7] = 0.0;
    
    projectionMatrix.data[8] = 2.0 * c_x / screen_width - 1.0;
    projectionMatrix.data[9] = 2.0 * c_y / screen_height - 1.0;
    projectionMatrix.data[10] = -( far+near ) / ( far - near );
    projectionMatrix.data[11] = -1.0;
    
    projectionMatrix.data[12] = 0.0;
    projectionMatrix.data[13] = 0.0;
    projectionMatrix.data[14] = -2.0 * far * near / ( far - near );
    projectionMatrix.data[15] = 0.0;

    return projectionMatrix;
}

// --------------------------------------------------------------------------
// idle(引数なし)
// プログラムのアイドル時に呼ばれる関数です
// 戻り値: なし
// --------------------------------------------------------------------------
void idle(void)
{
    // 再描画をリクエスト
    glutPostRedisplay();
}

// --------------------------------------------------------------------------
// display(引数なし)
// 画面の更新時に呼ばれる関数です
// 戻り値: なし
// --------------------------------------------------------------------------
void display(void)
{
    // 描画用のバッファクリア
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (cap.isOpened()) {
        // カメラ画像取得
        cv::Mat image;
        cap >> image;

        // カメラ画像(RGB)表示
        cv::Mat rgb;
        cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
        cv::flip(rgb, rgb, 0);
        glDepthMask(GL_FALSE);
        glDrawPixels(rgb.cols, rgb.rows, GL_RGB, GL_UNSIGNED_BYTE, rgb.data);

        // BGRA画像
        cv::Mat bgra;
        cv::cvtColor(image, bgra, cv::COLOR_BGR2BGRA);

        // データを渡す
        BGRAVideoFrame frame;
        frame.width = bgra.cols;
        frame.height = bgra.rows;
        frame.data = bgra.data;
        frame.stride = bgra.step;

        // マーカ検出
        MarkerDetector detector(calibration);
        detector.processFrame(frame);
        std::vector<Transformation> transformations = detector.getTransformations();

        // 射影変換行列を計算
        Matrix44 projectionMatrix = buildProjectionMatrix(calibration.getIntrinsic(), frame.width, frame.height);

        // 射影変換行列を適用
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projectionMatrix.data);

        // ビュー行列の設定
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // デプス有効
        glDepthMask(GL_TRUE);

        // 頂点配列有効
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        // ビュー行列を退避
        glPushMatrix();

        // ラインの太さ設定
        glLineWidth(3.0f);

        // ライン頂点配列
        float lineX[] = {0,0,0,1,0,0};
        float lineY[] = {0,0,0,0,1,0};
        float lineZ[] = {0,0,0,0,0,1};

        // 2D平面
        const GLfloat squareVertices[] = { -0.5f, -0.5f,
                                            0.5f, -0.5f,
                                           -0.5f,  0.5f,
                                            0.5f,  0.5f};

        // 2D平面カラー(RGBA)
        const GLubyte squareColors[] = {255, 255,   0, 255,
                                          0, 255, 255, 255,
                                          0,   0,   0,   0,
                                        255,   0, 255, 255};

        // AR描画
        for (size_t i = 0; i < transformations.size(); i++) {
            // 変換行列
            const Transformation &transformation = transformations[i];
            Matrix44 glMatrix = transformation.getMat44();

            // ビュー行列にロード
            glLoadMatrixf(reinterpret_cast<const GLfloat*>(&glMatrix.data[0]));

            // 2D平面の描画
            glEnableClientState(GL_COLOR_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, squareVertices);
            glColorPointer(4, GL_UNSIGNED_BYTE, 0, squareColors);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDisableClientState(GL_COLOR_ARRAY);

            // 座標軸のスケール
            float scale = 0.5;
            glScalef(scale, scale, scale);

            // カメラから見えるようにちょっと移動
            glTranslatef(0, 0, 0.1f);

            // X軸
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glVertexPointer(3, GL_FLOAT, 0, lineX);
            glDrawArrays(GL_LINES, 0, 2);

            // Y軸
            glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
            glVertexPointer(3, GL_FLOAT, 0, lineY);
            glDrawArrays(GL_LINES, 0, 2);

            // Z軸
            glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
            glVertexPointer(3, GL_FLOAT, 0, lineZ);
            glDrawArrays(GL_LINES, 0, 2);
        }

        // 頂点配列無効
        glDisableClientState(GL_VERTEX_ARRAY);

        // ビュー行列を戻す
        glPopMatrix();
    }

    // ダブルバッファリング
    glutSwapBuffers();
}

// --------------------------------------------------------------------------
// key(入力されたキー, マウスカーソルのx位置, y位置)
// キーボードの入力時に呼ばれる関数です
// 戻り値: なし
// --------------------------------------------------------------------------
void key(unsigned char key , int x , int y) {
    switch (key) {
        case 0x1b:
            exit(1);
            break;
        default :
            break;
    }
}

// --------------------------------------------------------------------------
// resize(画面の幅, 画面の高さ)
// 画面のリサイズ時に呼ばれる関数です
// 戻り値: なし
// --------------------------------------------------------------------------
void resize(int w, int h)
{
    // ビューポートを設定
    glViewport(0, 0, w, h);

    // 透視投影行列を再設定
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(30.0, (double)w / (double)h, 0.01, 100.0);
}

// --------------------------------------------------------------------------
// main(引数の数, 引数の内容)
// メイン関数です
// 戻り値: 成功:0  失敗:-1
// --------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // カメラ初期化
    if (!cap.open(0)) {
        printf("カメラ初期化エラー\n");
        return -1;
    }

    // ファイル名を指定して開く
    const char *filename = "camera.xml";
    FILE *file = fopen(filename, "r");

    // ファルがない
    if (!file) {
        // 画像
        std::vector<cv::Mat> images;
        std::cout << "Spaceキーを押して画像キャプチャ" << std::endl;
        std::cout << "Escで終了" << std::endl;

        // キャリブレーションのメインループ
        while (1) {
            // キー入力
            int key = cv::waitKey(1);
            if (key == 0x1b) break;

            // キャプチャ
            cv::Mat frame;
            cap >> frame;

            // グレースケール変換
            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

            // チェスボード検出
            cv::Size size(PAT_COL, PAT_ROW);
            std::vector<cv::Point2f> corners;
            bool found = cv::findChessboardCorners(gray, size, corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);

            // 見つかった
            if (found) {
                // チェスボード描画(するだけ)
                cv::drawChessboardCorners(frame, size, corners, found);

                // Spaceキーを押した
                if (key == ' ') {
                    // バッファに追加
                    images.push_back(gray);
                }
            }

            // 表示
            std::ostringstream stream;
            stream << "Captured " << images.size() << " image(s).";
            cv::putText(frame, stream.str(), cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0), 1, CV_AA);
            cv::imshow("Camera Calibration", frame);
        }

        // 十分なサンプルがある
        if (images.size() > 0) {
            // チェスボードのデータ
            cv::Size size(PAT_COL, PAT_ROW);
            std::vector<std::vector<cv::Point2f>> corners2D;
            std::vector<std::vector<cv::Point3f>> corners3D;

            for (size_t i = 0; i < images.size(); i++) {
                // チェスボード検出
                std::vector<cv::Point2f> tmp_corners2D;
                bool found = cv::findChessboardCorners(images[i], size, tmp_corners2D);

                // 見つかった
                if (found) {
                    //サブピクセル精度に変換
                    cv::cornerSubPix(images[i], tmp_corners2D, cvSize(11, 11), cvSize(-1, -1), cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 30, 0.1));
                    corners2D.push_back(tmp_corners2D);

                    // チェスボード実寸を代入
                    const float squareSize = CHESS_SIZE;
                    std::vector<cv::Point3f> tmp_corners3D;
                    for (int j = 0; j < size.height; j++ ) {
                        for (int k = 0; k < size.width; k++) {
                            tmp_corners3D.push_back(cv::Point3f((float)(j*squareSize), (float)(k*squareSize), 0.0));
                        }
                    }
                    corners3D.push_back(tmp_corners3D);
                }
            }

            // カメラキャリブレーション行列を推定
            cv::Mat cameraMatrix, distCoeffs;
            std::vector<cv::Mat> rvec, tvec;
            cv::calibrateCamera(corners3D, corners2D, images[0].size(), cameraMatrix, distCoeffs, rvec, tvec);
            std::cout << cameraMatrix << std::endl;
            std::cout << distCoeffs << std::endl;

            // 保存
            cv::FileStorage fs(filename, cv::FileStorage::WRITE);
            fs << "intrinsic" << cameraMatrix;
            fs << "distortion" << distCoeffs;
        }

        // ウィンドウをすべて破棄
        cv::destroyAllWindows();
    }
    else {
        // 閉じる
        fclose(file);
    }

    // XMLファイルを開く
    cv::FileStorage rfs(filename, cv::FileStorage::READ);
    if (!rfs.isOpened()) {
        std::cout << "カメラパラメータ読み込みに失敗しました" << std::endl;
        return -1;
    }

    // カメラパラメータ読み込み
    cv::Mat cameraMatrix, distCoeffs;
    rfs["intrinsic"] >> cameraMatrix;
    rfs["distortion"] >> distCoeffs;
    float fx = cameraMatrix.at<double>(0, 0);
    float fy = cameraMatrix.at<double>(1, 1);
    float cx = cameraMatrix.at<double>(0, 2);
    float cy = cameraMatrix.at<double>(1, 2);
    float dist[4] = {distCoeffs.at<double>(0), distCoeffs.at<double>(1), distCoeffs.at<double>(2), distCoeffs.at<double>(3)};
    calibration = CameraCalibration(fx, fy, cx, cy, dist);

    // GLUT初期化
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Mastering OpenCV with Practical Computer Vision Project");
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(resize);
    glutIdleFunc(idle);

    // シーン消去
    glClearColor(0.0, 0.0, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    // メインループ
    glutMainLoop();

    return 0;
}