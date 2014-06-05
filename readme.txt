Simple AR with OpenCV
A simple AR application using OpenCV and OpenGL.
Copyright (C) 2014 puku0x

「Mastering OpenCV with Practical Computer Vision Projects」第2章のOpenCVベースのマーカ有りAR(iPhone/iPad用)が面白そうだったのでWindows PCでも動かせるようにしてみた。
・OpenCVでマーカ検出+姿勢推定
・OpenGLでARのレンダリング
の他に
・OpenCVでカメラキャリブレーション
も追加した。

.\src\SimpleAR 内のファイルはここから拝借。
https://github.com/MasteringOpenCV/code/tree/master/Chapter2_iPhoneAR/Example_MarkerBasedAR/Example_MarkerBasedAR

いつものようにVisual Studioが入っている環境ならどこでもビルドできる親切設計。
Visual Studioの最新の更新プログラムを適用すれば大丈夫...なはず。
ビルド出来なかったら腹をくくって自分で作り直しましょう。

動作環境
  Windows 7 & Visual Studio 2008/2010/2012/2013

使用ライブラリ
  - OpenCV 2.4.9
    http://opencv.org
  - GLUT 3.7
    http://www.opengl.org/resources/libraries/glut/glut_downloads.php#windows

その他の注意事項
  プログラムの動作保証は致しかねます。
  また、質問は受け付けません。ごめんね。