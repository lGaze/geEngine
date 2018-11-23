#include <vld.h>
#include <DirectXMath.h>

#define GTEST_HAS_TR1_TUPLE 0
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gtest/gtest.h>

#include <gePrerequisitesUtil.h>
#include <geMath.h>
#include <geNumericLimits.h>
#include <geFileSystem.h>
#include <geDataStream.h>
#include <geFloat10.h>
#include <geFloat11.h>
#include <geFloat16.h>
#include <geFloat32.h>
#include <geVector2.h>
#include <geVector2I.h>
#include <geVector2Half.h>
#include <geVector3.h>
#include <geMatrix4.h>
#include <geTriangulation.h>
#include <geCompression.h>

#include <Win32/geWin32Windows.h>

#include <geDebug.h>

using namespace geEngineSDK;

TEST(geUtility, Basic_Type_Size) {
  EXPECT_EQ(sizeof(unsigned char), 1U);
  EXPECT_EQ(sizeof(uint8  ), 1U);
  EXPECT_EQ(sizeof(uint16 ), 2U);
  EXPECT_EQ(sizeof(uint32 ), 4U);
  EXPECT_EQ(sizeof(uint64 ), 8U);
  EXPECT_EQ(sizeof(uint128), 16U);
  
  EXPECT_EQ(sizeof(char  ), 1);
  EXPECT_EQ(sizeof(int8  ), 1);
  EXPECT_EQ(sizeof(int16 ), 2);
  EXPECT_EQ(sizeof(int32 ), 4);
  EXPECT_EQ(sizeof(int64 ), 8);
  EXPECT_EQ(sizeof(int128), 16);

  EXPECT_EQ(sizeof(float ), 4);
  EXPECT_EQ(sizeof(double), 8);

  EXPECT_EQ(static_cast<uint32>(-1), NumLimit::MAX_UINT32);
  EXPECT_FLOAT_EQ(Math::PI, 3.141592f);
}

TEST(geUtility, String_Basic) {
  String testString = "This is a test";
  EXPECT_EQ(testString.size(), 14);

  WString testWString = L"This is a test";
  EXPECT_EQ(testWString.size(), 14);

  EXPECT_EQ(toString(testWString).size(), 14);
  EXPECT_EQ(toWString(testString).size(), 14);

  StringUtil::toLowerCase(testString);
  EXPECT_STRCASEEQ(testString.c_str(), "this is a test");
  StringUtil::toUpperCase(testString);
  EXPECT_STRCASEEQ(testString.c_str(), "THIS IS A TEST");

  StringUtil::toLowerCase(testWString);
  ASSERT_EQ(testWString, L"this is a test");
  StringUtil::toUpperCase(testWString);
  ASSERT_EQ(testWString, L"THIS IS A TEST");

  //EXPECT_TRUE(StringUtil::match(testString, "*is", false));
}

TEST(geUtility, String_Path) {
  Path testPath;
  String lastDirectory;
  
  testPath = FileSystem::getWorkingDirectoryPath();
  EXPECT_TRUE(testPath.isDirectory());
  EXPECT_TRUE(testPath.getNumDirectories());

  lastDirectory.append(testPath[testPath.getNumDirectories() - 1]);
  EXPECT_TRUE(lastDirectory.compare("BIN"));

  Path tempPath = FileSystem::getTempDirectoryPath();
  tempPath = tempPath;
}

TEST(geUtility, String_Parser) {
  Path workingDirPath = FileSystem::getWorkingDirectoryPath();
  Path fromBinRoot = "Test/test.txt";
  Path fromBinConfig = "../Test/test.txt";
  DataStreamPtr fileData;

  if (FileSystem::exists(workingDirPath + fromBinRoot)) {
    fileData = FileSystem::openFile(fromBinRoot);
  }
  else if (FileSystem::exists(workingDirPath + fromBinConfig)) {
    fileData = FileSystem::openFile(fromBinConfig);
  }

  EXPECT_TRUE(fileData)
    << "Attempting to open a file that doesn't exist: Test/test.txt. "
    << workingDirPath.toString();

  if (fileData) {
    String strParse = fileData->getAsString();
    Vector<String> lineList = StringUtil::split(strParse, "\n");

    for (auto& line : lineList) {
      StringUtil::trim(line);
    }

    EXPECT_EQ(lineList.size(), 9);
  }
}

TEST(geUtility, Compression) {
  Path workingDirPath = FileSystem::getWorkingDirectoryPath();
  Path fromBinRoot = "Test/test.txt";
  Path fromBinConfig = "../Test/test.txt";
  DataStreamPtr fileData;

  if (FileSystem::exists(workingDirPath + fromBinRoot)) {
    fileData = FileSystem::openFile(fromBinRoot);
  }
  else if (FileSystem::exists(workingDirPath + fromBinConfig)) {
    fileData = FileSystem::openFile(fromBinConfig);
  }

  EXPECT_TRUE(fileData)
    << "Attempting to open a file that doesn't exist: Test/test.txt. "
    << workingDirPath.toString();
  
  if (fileData) {
    //Compress file data
    DataStreamPtr compressedFile = Compression::compress(fileData);
    EXPECT_TRUE(fileData->size() > compressedFile->size());

    //Decompress file data
    DataStreamPtr decompressedFile = Compression::decompress(compressedFile);

    String strFile = fileData->getAsString();
    String strFileDecomp = decompressedFile->getAsString();

    //Compare the information
    EXPECT_TRUE(std::memcmp(&strFile[0], &strFileDecomp[0], strFile.size()) == 0);
  }
}

TEST(geUtility, Math_Basic) {
  EXPECT_EQ(Math::abs(-1), 1);
  EXPECT_EQ(Math::abs(-1.000000000), 1.000000000);
  EXPECT_FLOAT_EQ(Math::abs(-1.0f), 1.0f);
  EXPECT_TRUE(Math::abs(Radian(-Math::PI)) == Radian(Math::PI));
  EXPECT_TRUE(Math::abs(Degree(-180.0f)) == Degree(180.0f));
  
  EXPECT_EQ(Math::ceil(1.000001f), 2);
  EXPECT_EQ(Math::ceil(1.00000001f), 1);
  EXPECT_EQ(Math::ceilFloat(1.000001f), 2.0f);
  EXPECT_EQ(Math::ceilFloat(1.00000001f), 1.0f);
  EXPECT_EQ(Math::ceilDouble(1.000000000000001), 2.0);
  EXPECT_EQ(Math::ceilDouble(1.0000000000000001), 1.0);

  EXPECT_EQ(Math::floor(1.9999999f), 1);
  EXPECT_EQ(Math::floor(1.99999999f), 2);
  EXPECT_EQ(Math::floorFloat(1.9999999f), 1.0f);
  EXPECT_EQ(Math::floorFloat(1.99999999f), 2.0f);
  EXPECT_EQ(Math::floorDouble(1.999999999999999), 1.0);
  EXPECT_EQ(Math::floorDouble(1.9999999999999999), 2.0);

  uint32 ceilTmp[] = {0, 0, 1, 2, 2, 3, 3, 3, 3, 4};
  uint32 floorTmp[] = {0, 0, 1, 1, 2, 2, 2, 2, 3, 3};
  for (uint32 i = 0; i < 10; ++i) {
    EXPECT_EQ(Math::ceilLog2(i), ceilTmp[i]);
    EXPECT_EQ(Math::floorLog2(i), floorTmp[i]);
  }

  EXPECT_EQ(Math::clamp(100.0, 1.0, 50.0), 50);
  EXPECT_EQ(Math::clamp(18.0, 1.0, 50.0),18.0);
  EXPECT_EQ(Math::clamp(-9.0, 1.0, 50.0), 1.0);

  EXPECT_EQ(Math::clamp01(100.0f), 1.0f);
  EXPECT_EQ(Math::clamp01(18.0f),  1.0f);
  EXPECT_EQ(Math::clamp01(-9.0f),  0.0f);

  EXPECT_NEAR(Math::fmod(18.5f, 4.2f), 1.7f, Math::SMALL_NUMBER);
  EXPECT_NEAR(Math::exp(5), 148.413159f, Math::SMALL_NUMBER);
  EXPECT_NEAR(Math::fractional(3.141592653f), 0.141592653f, Math::SMALL_NUMBER);

  EXPECT_FLOAT_EQ(Math::sqrt(Math::square(5.f)), 5.f);
  EXPECT_FLOAT_EQ(Math::invSqrt(5.f) * 5.f, Math::sqrt(5.f));
  EXPECT_NEAR(Math::invSqrtEst(5.f) * 5.f, Math::sqrt(5.f), Math::DELTA);

  EXPECT_TRUE(Math::isWithin(0.1f, 0.0f, 10.0f));
  EXPECT_TRUE(Math::isWithin(5.0f, 0.0f, 10.0f));
  EXPECT_TRUE(Math::isWithin(9.9f, 0.0f, 10.0f));
  EXPECT_TRUE(Math::isWithin(0.0f, 0.0f, 10.0f));
  EXPECT_FALSE(Math::isWithin(-5.f, 0.0f, 10.0f));
  EXPECT_FALSE(Math::isWithin(10.f, 0.0f, 10.0f));
  EXPECT_TRUE(Math::isWithinInclusive(10.f, 0.0f, 10.0f));
}

TEST(geUtility, Math_Bit_Checks) {
  EXPECT_TRUE(Math::isPowerOfTwo(0));
  EXPECT_TRUE(Math::isPowerOfTwo(1));
  EXPECT_TRUE(Math::isPowerOfTwo(2));
  EXPECT_TRUE(Math::isPowerOfTwo(4));
  EXPECT_TRUE(Math::isPowerOfTwo(8));
  EXPECT_TRUE(Math::isPowerOfTwo(16));
  EXPECT_TRUE(Math::isPowerOfTwo(32));
  EXPECT_TRUE(Math::isPowerOfTwo(64));
  EXPECT_TRUE(Math::isPowerOfTwo(128));
  EXPECT_TRUE(Math::isPowerOfTwo(256));

  EXPECT_FALSE(Math::isPowerOfTwo(3));
  EXPECT_FALSE(Math::isPowerOfTwo(5));
  EXPECT_FALSE(Math::isPowerOfTwo(9));
  EXPECT_FALSE(Math::isPowerOfTwo(33));
  EXPECT_FALSE(Math::isPowerOfTwo(65));
  EXPECT_FALSE(Math::isPowerOfTwo(129));
  EXPECT_FALSE(Math::isPowerOfTwo(257));
  EXPECT_FALSE(Math::isPowerOfTwo(513));
}

TEST(geUtility, Math_Float_Precision) {
  float negativeFloat = -1.0f;
  
  EXPECT_FALSE(Math::isNaN(0.0f));
  EXPECT_TRUE(Math::isNaN(Math::sqrt(negativeFloat)));

  EXPECT_TRUE(Math::isFinite(0.0f));
  EXPECT_FALSE(Math::isFinite(Math::sqrt(negativeFloat)));

  EXPECT_TRUE(Math::isNearlyEqual(1.999999f, 2.0f));
  EXPECT_FALSE(Math::isNearlyEqual(1.99999f, 2.0f));
  EXPECT_TRUE(Math::isNearlyEqual(1.99991f, 2.0f, Math::KINDA_SMALL_NUMBER));
  EXPECT_FALSE(Math::isNearlyEqual(1.9999f, 2.0f, Math::KINDA_SMALL_NUMBER));

  EXPECT_FALSE(Math::isNearlyZero(0.000001));
  EXPECT_TRUE(Math::isNearlyZero(0.000000000000001));
  EXPECT_FALSE(Math::isNearlyZero(0.0001f, Math::KINDA_SMALL_NUMBER));
  EXPECT_TRUE(Math::isNearlyZero(0.00001f, Math::KINDA_SMALL_NUMBER));

  Float32 floatTstValue32(152.0f);

  Float16 floatTstValue16(floatTstValue32.floatValue);
  Float11 floatTstValue11(floatTstValue16.getFloat());
  Float10 floatTstValue10(floatTstValue16.getFloat());

  Float32 floatOutValue16(floatTstValue16.getFloat());
  Float32 floatOutValue11(floatTstValue11.getFloat());
  Float32 floatOutValue10(floatTstValue10.getFloat());

  EXPECT_FLOAT_EQ(floatOutValue16.floatValue, floatTstValue32.floatValue);
  EXPECT_FLOAT_EQ(floatOutValue11.floatValue, floatTstValue32.floatValue);
  EXPECT_FLOAT_EQ(floatOutValue10.floatValue, floatTstValue32.floatValue);
}

TEST(geUtility, Math_Trigonometric) {
  EXPECT_FLOAT_EQ(Math::sin(0.f), 0.0f);
  EXPECT_FLOAT_EQ(Math::cos(0.f), 1.0f);
  EXPECT_FLOAT_EQ(Math::tan(Math::HALF_PI*0.5f), 1.0f);

  EXPECT_NEAR(Math::acos(0.f).valueRadians(), 1.57070000f, Math::KINDA_SMALL_NUMBER);
  EXPECT_NEAR(Math::acos(0.f).valueRadians(), 1.57079600f, Math::SMALL_NUMBER);
  EXPECT_NEAR(Math::acos(0.f).valueRadians(), 1.57079633f, 0);

  EXPECT_NEAR(Math::asin(1.f).valueRadians(), 1.57070000f, Math::KINDA_SMALL_NUMBER);
  EXPECT_NEAR(Math::asin(1.f).valueRadians(), 1.57079600f, Math::SMALL_NUMBER);
  EXPECT_NEAR(Math::asin(1.f).valueRadians(), 1.57079633f, 0);

  EXPECT_NEAR(Math::atan(1.f).valueRadians(), 0.785390000f, Math::KINDA_SMALL_NUMBER);
  EXPECT_NEAR(Math::atan(1.f).valueRadians(), 0.785398100f, Math::SMALL_NUMBER);
  EXPECT_NEAR(Math::atan(1.f).valueRadians(), 0.785398185f, 0);

  EXPECT_FLOAT_EQ(Math::atan2(1.f, 1.f).valueRadians(), Math::HALF_PI*0.5f);
  EXPECT_FLOAT_EQ(Degree(Math::atan2(1.f, 1.f)).valueDegrees(), 45.0f);

  EXPECT_FLOAT_EQ(Math::atan2(-1.f, 1.f).valueRadians(), -(Math::HALF_PI*0.5f));
  EXPECT_FLOAT_EQ(Degree(Math::atan2(-1.f, 1.f)).valueDegrees(), -45.0f);

  EXPECT_FLOAT_EQ(Math::atan2(1.f, -1.f).valueRadians(), Math::PI*0.75f);
  EXPECT_FLOAT_EQ(Degree(Math::atan2(1.f, -1.f)).valueDegrees(), 135.0f);

  EXPECT_FLOAT_EQ(Math::atan2(-1.f, -1.f).valueRadians(), -(Math::PI*0.75f));
  EXPECT_FLOAT_EQ(Degree(Math::atan2(-1.f, -1.f)).valueDegrees(), -135.0f);
}

TEST(geUtility, Math_Fast) {
  float tstValR, tstVal0, tstVal1;

  tstVal0 = Math::fastSin0(Degree(45.0f).valueRadians());
  tstVal1 = Math::fastSin1(Degree(45.0f).valueRadians());
  tstValR = Math::sin(Degree(45.0f).valueRadians());
  EXPECT_NEAR(tstValR, tstVal0, 0.001f);
  EXPECT_NEAR(tstValR, tstVal1, 0.00001f);

  tstVal0 = Math::fastCos0(Degree(45.0f).valueRadians());
  tstVal1 = Math::fastCos1(Degree(45.0f).valueRadians());
  tstValR = Math::cos(Degree(45.0f).valueRadians());
  EXPECT_NEAR(tstValR, tstVal0, 0.001f);
  EXPECT_NEAR(tstValR, tstVal1, 0.00001f);

  tstVal0 = Math::fastTan0(Degree(45.0f).valueRadians());
  tstVal1 = Math::fastTan1(Degree(45.0f).valueRadians());
  tstValR = Math::tan(Degree(45.0f).valueRadians());
  EXPECT_NEAR(tstValR, tstVal0, 0.001f);
  EXPECT_NEAR(tstValR, tstVal1, 0.00001f);

  tstVal0 = Math::fastASin0(Degree(45.0f).valueRadians());
  tstVal1 = Math::fastASin1(Degree(45.0f).valueRadians());
  tstValR = Math::asin(Degree(45.0f).valueRadians()).valueRadians();
  EXPECT_NEAR(tstValR, tstVal0, 0.001f);
  EXPECT_NEAR(tstValR, tstVal1, 0.00001f);

  tstVal0 = Math::fastACos0(Degree(45.0f).valueRadians());
  tstVal1 = Math::fastACos1(Degree(45.0f).valueRadians());
  tstValR = Math::acos(Degree(45.0f).valueRadians()).valueRadians();
  EXPECT_NEAR(tstValR, tstVal0, 0.001f);
  EXPECT_NEAR(tstValR, tstVal1, 0.00001f);

  tstVal0 = Math::fastATan0(Degree(45.0f).valueRadians());
  tstVal1 = Math::fastATan1(Degree(45.0f).valueRadians());
  tstValR = Math::atan(Degree(45.0f).valueRadians()).valueRadians();
  EXPECT_NEAR(tstValR, tstVal0, 0.001f);
  EXPECT_NEAR(tstValR, tstVal1, 0.00001f);
}

TEST(geUtility, Math_Vector2I) {
  Vector2I tmpVector0(5);
  Vector2I tmpVector1(3);
  Vector2I tmpVector2(3, 4);

  int32 value = tmpVector2 | tmpVector0;
  EXPECT_TRUE(35 == value);
}

TEST(geUtility, Math_Vector2Half) {
  Vector2 tmpVector0(15.f, 15.f);
  Vector2 tmpVector1;
  Vector2Half tmpHalf;
  
  tmpHalf = tmpVector0;
  tmpVector1 = tmpHalf;

  EXPECT_FLOAT_EQ(tmpVector1.x, 15.f);
  EXPECT_FLOAT_EQ(tmpVector1.y, 15.f);
}

TEST(geUtility, Math_Intersections) {
  EXPECT_TRUE(Math::lineLineIntersection( Vector2(0.f, 5.f),
                                          Vector2(15.f, 5.f),
                                          Vector2(10.f, 0.f),
                                          Vector2(10.f, 10.f)));

  Vector2 intersectPoint = Math::getLineLineIntersect(Vector2(0.f, 5.f),
                                                      Vector2(15.f, 5.f),
                                                      Vector2(10.f, 0.f),
                                                      Vector2(10.f, 10.f));

  EXPECT_FLOAT_EQ(intersectPoint.x, 10.f);
  EXPECT_FLOAT_EQ(intersectPoint.y, 5.f);
}

/*
TEST(geUtility, Platform_Utilities) {
  PlatformUtility::copyToClipboard(L"Esta es una prueba del portapapeles!");
  WString szClipboardContent = PlatformUtility::copyFromClipboard();
  EXPECT_STRCASEEQ(toString(szClipboardContent).c_str(),
                   "Esta es una prueba del portapapeles!");
}
*/

TEST(geUtility, Tetrahedron_Utilities) {
  Vector<Vector3> pointList;
  pointList.emplace_back(-5.0f, 0.0f, 5.0f);
  pointList.emplace_back( 5.0f, 0.0f, 5.0f);
  pointList.emplace_back( 0.0f, 0.0f, 0.0f);
  pointList.emplace_back( 0.0f, 5.0f, 0.0f);

  TetrahedronVolume newVolume = Triangulation::tetrahedralize(pointList);
  EXPECT_TRUE(newVolume.tetrahedra.size() == 1);
  EXPECT_TRUE(newVolume.outerFaces.size() == 4);
}

TEST(geUtility, Matrix4_LookAt) {
  //Construct a DirectX LookAt Matrix
  DirectX::XMVECTOR EyePosition    = DirectX::XMVectorSet(0.0f, -100.0f, 0.0f, 1.0f);
  DirectX::XMVECTOR FocusPosition  = DirectX::XMVectorSet(0.0f,    0.0f, 0.0f, 1.0f);
  DirectX::XMVECTOR UpDirection    = DirectX::XMVectorSet(0.0f,    0.0f, 1.0f, 0.0f);
  DirectX::XMMATRIX lookAt         = DirectX::XMMatrixLookAtLH(EyePosition,
                                                               FocusPosition,
                                                               UpDirection);

  //Construct the same matrix with internal Math
  Vector3 iEyePosition(0.0f, -100.0f, 0.0f);
  Vector3 iFocusPosition(0.0f, 0.0f, 0.0f);
  Vector3 iUpDirection(0.0f, 0.0f, 1.0f);
  LookAtMatrix ilookAt(iEyePosition, iFocusPosition, iUpDirection);

  for (size_t j = 0; j < 4; ++j) {
    for (size_t k = 0; k < 4; ++k) {
      EXPECT_FLOAT_EQ(ilookAt.m[j][k], DirectX::XMVectorGetByIndex(lookAt.r[j], k));
    }
  }
}

TEST(geUtility, Matrix4_Perspective) {
  float FovAngleY = Degree(60).valueRadians();
  float AspectRatio = 1920.0f / 1080.0f;
  float NearZ = 1.0f;
  float FarZ = 1000.0f;
  
  //Construct a DirectX Perspective Matrix
  DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(FovAngleY,
                                                                   AspectRatio,
                                                                   NearZ,
                                                                   FarZ);

  //Construct the same matrix with internal Math
  PerspectiveMatrix iprojection(FovAngleY * 0.5f, 1920.0f, 1080.0f, NearZ, FarZ);

  EXPECT_FLOAT_EQ(iprojection.m[1][1] / iprojection.m[0][0],
                  DirectX::XMVectorGetByIndex(projection.r[1], 1) /
                  DirectX::XMVectorGetByIndex(projection.r[0], 0));

  EXPECT_FLOAT_EQ(iprojection.m[2][3], DirectX::XMVectorGetByIndex(projection.r[2], 3));
  EXPECT_FLOAT_EQ(iprojection.m[3][2], DirectX::XMVectorGetByIndex(projection.r[3], 2));
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  HDC hdc;
  PAINTSTRUCT ps;

  switch (msg)
  {
    case WM_LBUTTONDOWN:
      break;
    case WM_PAINT:
      hdc = BeginPaint(hwnd, &ps);
      EndPaint(hwnd, &ps);
      return 0;
    case WM_CLOSE:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

TEST(geUtility, FrameAllocTest) {
  WINDOW_DESC windowDesc;
  windowDesc.showTitleBar = true;
  windowDesc.showBorder = true;
  windowDesc.allowResize = true;
  windowDesc.enableDoubleClick = true;
  windowDesc.fullscreen = false;
  windowDesc.width = 1920;
  windowDesc.height = 1080;
  windowDesc.hidden = false;
  windowDesc.left = 0;
  windowDesc.top = 0;
  windowDesc.outerDimensions = false;
  windowDesc.title = "wndTest";
  windowDesc.toolWindow = false;
  windowDesc.creationParams = nullptr;
  windowDesc.modal = true;
  windowDesc.wndProc = &WndProc;
  windowDesc.module = GetModuleHandle(nullptr);
  
  SPtr<Win32Window> m_appWnd = ge_shared_ptr_new<Win32Window>(windowDesc);
}
