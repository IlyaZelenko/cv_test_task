#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <cmath>

using namespace cv;
using namespace std;


#define PI 3.14159265358979323846


inline double rad2Deg(double rad) { return rad * (180.0 / PI); }
inline double deg2Rad(double deg) { return deg * (PI / 180.0); }

void rotate(Point2f& point, double angle){
    double tmpX = point.x;
    double tmpY = point.y;
    point.x = tmpX*cos(angle)-tmpY*sin(angle);
    point.y = tmpX*sin(angle)+tmpY*cos(angle);
}

Point2f* getProjectedPoints( double alt, double rollAbs, double pitch, double yaw, double vfov, double hfov, Mat& image){
    Point2f* projectedPoints = new Point2f[4];
    
    double rolledHfov = cos(rollAbs)*hfov + sin(rollAbs)*vfov;
    double rolledVfov = cos(rollAbs)*vfov + sin(rollAbs)*hfov;
    //Point3f forward(0.f, 1.f, 0.f); //idle drone direction, looking to the North

    double lowerAng = PI/2-(rolledVfov/2 - pitch);
    double upperAngle = ((PI/2 + pitch) + lowerAng)/2;
    //cout<<"Lower Angle: "<<rad2Deg(lowerAng)<<'\t'<<"upper angle: "<<rad2Deg(upperAngle)<<'\n';
    double rightAng  = rolledHfov/2;     //rolledHfov/2
    double leftAng   = -rightAng;        //-rolledHfov/2
    
    double lowerY   = alt*(tan(lowerAng));  //cos(leftAng)*alt*(1/tan(lowerAng));
    double leftLowerX    = tan(leftAng)*lowerY;
    double rightLowerX   = -leftLowerX;     //tan(rightAng)*lowerY
    
    double upperY = alt*(tan(upperAngle));
    double leftUpperX = tan(leftAng)*alt*(tan(upperAngle));
    double rightUpperX = -leftUpperX;       //tan(rightAng)*alt*(tan(upperAngle))

    Point2f leftLower  = Point2f(leftLowerX, lowerY);
    rotate(leftLower, yaw);
    //cout<<"Projected Points: "<<"\n\t"<<leftLower.x<<", "<<leftLower.y<<'\n';
    leftLower.x += 400; leftLower.y = 400-leftLower.y;
    Point2f rightLower = Point2f(rightLowerX, lowerY);
    rotate(rightLower, yaw);
    //cout<<"\t"<<rightLower.x<<", "<<rightLower.y<<'\n';
    rightLower.x += 400; rightLower.y = 400-rightLower.y;
    Point2f leftUpper  = Point2f(leftUpperX,  upperY);
    rotate(leftUpper, yaw);
    //cout<<"\t"<<leftUpper.x<<", "<<leftUpper.y<<'\n';
    leftUpper.x += 400; leftUpper.y = 400-leftUpper.y;

    Point2f rightUpper = Point2f(rightUpperX, upperY); 
    rotate(rightUpper, yaw);
    //cout<<"\t"<<rightUpper.x<<", "<<rightUpper.y<<'\n';
    rightUpper.x += 400; rightUpper.y = 400-rightUpper.y;

    double max_distance = image.rows*( 1 - ( atan(sqrt(2)*400/alt) - lowerAng )/rolledVfov);
    //cout<<"coefficient"<< 1 - ( atan(sqrt(2)*400/alt) - lowerAng )/rolledVfov <<"\tmax_distance: "<<max_distance<<'\n';
    rectangle(image, Point(0,0), Point(image.cols-1, max_distance), Scalar(0), FILLED);

    projectedPoints[0] = leftUpper;
    projectedPoints[1] = rightUpper;
    projectedPoints[2] = leftLower;
    projectedPoints[3] = rightLower;

    return projectedPoints;
}



int main( int argc, char** argv )
{
    CommandLineParser parser( argc, argv, "{@input || input image}" );
    Mat src = imread( samples::findFile( parser.get<String>( "@input" ) ) );
    if( src.empty() )
    {
        cout << "Could not open or find the image!\n" << endl;
        cout << "Usage: " << argv[0] << " <Input image>" << endl;
        return -1;
    }

    
    double hfov = deg2Rad(60);       
    double vfov = deg2Rad(45);       
    double altitude = 50;   //meters
    double pitch = deg2Rad(-22);
    double roll = deg2Rad(-2);
    double yaw = deg2Rad(0);

    double rollAbs = abs(roll);
    double diagonal = sqrt(src.rows*src.rows + src.cols*src.cols);
    
    double scale = 1;
    double angleForX = atan(src.rows/double(src.cols));
    double angleForY = atan(src.cols/double(src.rows));

    //for new borders of the image after roll
    int adjustedX = round( cos(rollAbs-angleForX)*diagonal );  
    int adjustedY = round( cos(angleForY-rollAbs)*diagonal );
    
    //
    //cout<<"src (x, y): ("<< src.cols  << ", " << src.rows << ")\n";
    //cout<<"adjusted (x, y): (" << adjustedX << ", " << adjustedY << ")\n";
    //cout<<"angle between the diagonals: " << rad2Deg(atan(src.rows/double(src.cols))) << '\n';
    //
    

    Mat rotate_dst;
    Mat margined;
    int top = (adjustedY-src.rows)/2; int bottom = top;
    int left = (adjustedX-src.cols)/2; int right = left; 
    
    //
    //cout << "borders (top, left): ("<< top << ", " << left << ")\n";
    //   

    copyMakeBorder( src, margined, top, bottom, left, right, BORDER_CONSTANT);
    Point center = Point( adjustedX/2, adjustedY/2 );
    Mat rot_mat = getRotationMatrix2D( center, rad2Deg(roll), scale );
    warpAffine( margined, rotate_dst, rot_mat, margined.size() );
    
    //
    //cout << "center point: " << adjustedX/2 << ", " << adjustedY/2 << "\n";
    //Scalar red = Scalar(0, 0, 255);
    //Point srcCenter = Point(src.cols/2, src.rows/2);
    //circle( src, srcCenter, 1, red);
    //circle(margined, center, 1, red);    
    //circle(rotate_dst, center, 1, red);
    //

    Point2f srcQuad[4];
    srcQuad[0] = Point2f( 0.f                       , 3*rotate_dst.rows/4.f );
    srcQuad[1] = Point2f( rotate_dst.cols - 1.f     , 3*rotate_dst.rows/4.f );
    srcQuad[2] = Point2f( 0.f                       , rotate_dst.rows - 1.f );
    srcQuad[3] = Point2f( rotate_dst.cols - 1.f     , rotate_dst.rows - 1.f );
    
    //
    //cout<<"Points on the image:\n\t"<<srcQuad[0].x<<", "<<srcQuad[0].y<<'\t'<<srcQuad[1].x<<", "<<srcQuad[1].y<<'\t'<<srcQuad[2].x<<", "<<srcQuad[2].y<<'\t'<<srcQuad[3].x<<", "<<srcQuad[3].y<<'\t'<<'\n';
    //
    
    Mat warp_rotate_dst = Mat::zeros( 800, 800, src.type() );

    Point2f* dstQuad = getProjectedPoints(altitude, rollAbs, pitch, yaw, vfov, hfov, rotate_dst);
    
    Mat warp_mat = getPerspectiveTransform( srcQuad, dstQuad );
    
    warpPerspective( rotate_dst, warp_rotate_dst, warp_mat, warp_rotate_dst.size() );
    
    circle(warp_rotate_dst, Point(400, 400), 2, Scalar(255, 255, 255));    //drone point in the center of the image
    
    //
    //imshow( "Source image", src );
    //imshow( "Bordered", margined );
    //imwrite( "./assets/untiltedRoad.png", warp_rotate_dst );
    //

    imwrite( "./assets/projectedImage.png", warp_rotate_dst);

    return 0;
}