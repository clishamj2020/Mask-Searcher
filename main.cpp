
// COPYRIGHT 2023 Michael Clisham
// Worked together with Serena Owens
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <omp.h>
#include "PNG.h"
// It is ok to use the following namespace delarations in C++ source
// files only. They must never be used in header files.
using namespace std;
using namespace std::string_literals;

bool customSort(const std::string& a, const std::string& b) {
    // Split strings into tokens using ',' as delimiter
    std::vector<int> tokensA, tokensB;

    std::istringstream issA(a), issB(b);
    int num;

    while (issA >> num) {
        tokensA.push_back(num);
        if (issA.peek() == ',') {
            issA.ignore();
        }
    }

    while (issB >> num) {
        tokensB.push_back(num);
        if (issB.peek() == ',') {
            issB.ignore();
        }
    }

    // Compare corresponding elements in the vectors
    for (size_t i = 0; i < std::min(tokensA.size(), tokensB.size()); ++i) {
        if (tokensA[i] < tokensB[i]) {
            return true;
        } else if (tokensA[i] > tokensB[i]) {
            return false;
        }
    }

    // If all corresponding elements are equal, compare based on the length
    // should never get to here since there are no repeats.
    return tokensA.size() < tokensB.size();
}

/**
 * Method to replace instances of spaces with commas
 * 
 * \param[in] str The string to replace the spaces with commas within. 
 * 
 * \return A string with the commas replaced
 */
string replaceWithComma(string str) {
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == ' ') {
            str.replace(i, 1, ", ");
            ++i;
        }
    }
    return str;
}
/**
 * Method to print the matches found
 * 
 * \param[in] matches The vector of strings containing the coordinates
 * where a match occured.
*/
void printMatches(const vector<string> & matches) {
    int matchCount = 0;
    for (const auto& match : matches) {
    std::cout << "sub-image matched at: " << replaceWithComma(match)
        << std::endl;
        ++matchCount;
    }
    std::cout << "Number of matches: " << matchCount << std::endl;
}
/**
 * Method to return the average background pixel color from a given region
 * 
 * \param[in] img1 The PNG object containing the main image
 * 
 * \param[in] mask The PNG object containing the mask image
 * 
 * \param[in] startRow The starting row of the region
 * 
 * \param[in] startCol The starting column of the region
 * 
 * \param[in] maxRow The ending row of the region
 * 
 * \param[in] maxCol The ending row of the column
 * 
 * \return A Pixel object with the average background color of a region
*/
Pixel computeBackgroundPixel(const PNG& img1, const PNG& mask,
                             const int startRow, const int startCol,
                             const int maxRow, const int maxCol) {
    const Pixel Black{ .rgba = 0xff'00'00'00U };
    int red = 0, blue = 0, green = 0, count = 0;
    // Loop through pixels
    for (int row = 0; row < maxRow; ++row) {
        for (int col = 0; col < maxCol; ++col) {
            // If pixel's color equal Black pixel's color
            if (mask.getPixel(row, col).rgba == Black.rgba) {
                const auto pix = img1.getPixel(row + startRow, col + startCol);
                // Add to color totals
                red += pix.color.red;
                green += pix.color.green;
                blue += pix.color.blue;
                ++count;
            }
        }
    }
    // Compute and return average pixel color
    const unsigned char avgRed = (red / count),
                        avgGreen = (green / count),
                        avgBlue = (blue / count);
    return {.color = {avgRed, avgGreen, avgBlue, 0}};
}
/**
 * Method to compute and return the netMatch score of pixels in a region
 * compared to the mask image's background
 * 
 * \param[in] region The coordinates of the region to compute netMatch
 * 
 * \param[in] tol The tolerance at which it is okay to consider a pixel matched
 * 
 * \param[in] mainIMG the PNG object containing the main image
 * 
 * \param[in] maskIMG the PNG object containing the mask image
 * 
 * \return The net match count of pixels that match the background
*/
int getMatchCount(const string region, const int tol, PNG& mainIMG, 
    PNG& maskIMG) {
        // Read in region's vars with a string stream
        stringstream read(region);
        int x, y, xEnd, yEnd;
        read >> x >> y >> xEnd >> yEnd;
        // Compute the background pixel of a region
        Pixel background = computeBackgroundPixel(mainIMG, maskIMG,
            x, y, maskIMG.getHeight(), maskIMG.getWidth());
        int matchPixCount = 0, mismatchPixCount = 0;
        const Pixel Black{ .rgba = 0xff'00'00'00U };
        // Loop through pixels
        for (int row = 0; (row < maskIMG.getHeight()); row++) {
            for (int col = 0; (col < maskIMG.getWidth()); col++) {
                // Assign current pixel
                const auto pix = mainIMG.getPixel(row + x, col + y);
                // If difference of current pixel and background pixel's colors
                // is less than the tolerance, it's a match.
                if (abs(pix.color.red - background.color.red) < tol 
                    && abs(pix.color.green - background.color.green) < tol 
                    && abs(pix.color.blue - background.color.blue) < tol) {
                        if (maskIMG.getPixel(row, col).rgba == Black.rgba) {
                            matchPixCount++;
                        } else {
                            mismatchPixCount++;
                        }
                // If background pixel isn't within the tolerance, it's still
                // a mismatch
                } else {
                    if (maskIMG.getPixel(row, col).rgba == Black.rgba) {
                        mismatchPixCount++;
                    } else {
                        matchPixCount++;
                    }
                }
            }
        }
        return (matchPixCount - mismatchPixCount);
    }
/**
 * Method to generate the coordinates of each region within the main image
 * 
 * \param[in] maskWidth The width of the region
 * 
 * \param[in] maskHeight The height of the region
 * 
 * \param[in] mainWidth The width of the main image
 * 
 * \param[in] mainHeight The height of the main image
 * 
 * \return 
*/
vector<string> genCords(const int maskWidth, const int maskHeight,
    const int mainWidth, const int mainHeight) {
        vector<string> coords;
        // Loop through the pixels and generate coords
        for (int x = 0; x < mainHeight - maskHeight + 1; ++x) {
            for (int y = 0; y < mainWidth - maskWidth + 1; ++y) {
                int xEnd = x + maskHeight;
                int yEnd = y + maskWidth;
                // Add coords to vector of coords
                coords.push_back(to_string(x) + " " + to_string(y) +
                    " " + to_string(xEnd) + " " + to_string(yEnd));
            }
        }
        return coords;
    }
/**
 * Method to check if two regions overlap
 * 
 * \param[in] topLeftX The top left x-coordinate of 1st region
 * 
 * \param[in] topLeftY The top left y-coordinate of 1st region
 * 
 * \param[in] bottomRightX The bottom right x-coordinate of 1st region
 * 
 * \param[in] bottomRightY The bottom right y-coordinate of 1st region
 *
 * \param[in] topLeftX2 The top left x-coordinate of 2nd region
 * 
 * \param[in] topLeftY2 The top left y-coordinate of 2nd region
 * 
 * \param[in] bottomRightX2 The bottom right x-coordinate of 2nd region
 * 
 * \param[in] bottomRightY2 The bottom right y-coordinate of 2nd region
*/
bool isOverlap(const int topLeftX, const int topLeftY, const int bottomRightX,
    const int bottomRightY, const int topLeftX2, const int topLeftY2, 
    const int bottomRightX2, const int bottomRightY2) {
        if (bottomRightX < topLeftX2 || bottomRightX2 < topLeftX) {
            return false;
        }
        if (bottomRightY < topLeftY2 || bottomRightY2 < topLeftY) {
            return false;
        }
        return true;
    }
/**
 * Method to check if a region should be added to the matches vector
 * 
 * \param[in] matches The vector of coordinates that match the background
 * 
 * \param[in] region A string of coordinates of a region we are checking
 * 
 * \return A bool that is true if we should add the region to the vector
*/
bool checkAdd(vector<string> matches, string region) {
    bool add = true;
    for (size_t i = 0; i < matches.size(); i++) {
        stringstream read1(matches.at(i));
        stringstream read2(region);
        // Read in region coords
        int topLeftX, topLeftY, bottomRightX, bottomRightY, 
            topLeftX2, topLeftY2, bottomRightX2, bottomRightY2;
        read1 >> topLeftX >> topLeftY >> bottomRightX 
            >> bottomRightY;
        read2 >> topLeftX2 >> topLeftY2 >> bottomRightX2 
            >> bottomRightY2;
        // Check if they overlap
        if (isOverlap(topLeftX, topLeftY, bottomRightX,
            bottomRightY, topLeftX2, topLeftY2, bottomRightX2,
            bottomRightY2)) {
                add = false;
                break;
            }
    }
    return add;
}
/**
 * Method to add a matched region to the matches vector
 * 
 * \param[in] region A string of coordinates of a matched region
 * 
 * \param[in] matches The vector of coordinates that match the background
*/
void addToMatches(string region, vector<string>& matches) {
    int size = matches.size();
    if (size == 0) {
        matches.push_back(region);
    } else {
        bool add = checkAdd(matches, region);
        // If we can add to matches; add to matches
        if (add) {
            matches.push_back(region);
        }
    }
}
/**
 * Method supplied to us that draws a red box around a specified region
 * 
 * \param[in] png The PNG object to write the box onto
 * 
 * \param[in] row The row number of where to right the box
 * 
 * \param[in] col The col number of where to right the box
 * 
 * \param[in] width The width of the box to be written
 * 
 * \param[in] height The height of the box to be written
*/
void drawBox(PNG& png, int row, int col, int width, int height) {
    // Draw horizontal lines
    for (int i = 0; (i < width); i++) {
        png.setRed(row, col + i);
        png.setRed(row + height, col + i);
    }
    // Draw vertical lines
    for (int i = 0; (i < height); i++) {
        png.setRed(row + i, col);
        png.setRed(row + i, col + width);
    }
}
/**
 * Method to draw all of the boxes to the final image and write a PNG file
 * 
 * \param[in] mainImgageFile A string containing the name of the main image file
 * 
 * \param[in] maskIMG A PNG object of the mask image
 * 
 * \param[in] matches The vector of coordinates that match the background
 * 
 * \param[in] outImageFile A string containing the name of the output file
*/
void drawFinal(const string & mainImageFile, PNG maskIMG,
    const vector<string> & matches, const string & outImageFile) {
    PNG outIMG;
    // Set pixels in output image to equal pixels in main image
    outIMG.load(mainImageFile);
    for (const auto & match : matches) {
        int x, y, xEnd, yEnd;
        stringstream read(match);
        read >> x >> y >> xEnd >> yEnd;
        // For each match, draw a box around it
        drawBox(outIMG, x, y, maskIMG.getWidth() - 1, maskIMG.getHeight() - 1);
    }
    outIMG.write(outImageFile);
}
/**
 * This is the top-level method that is called from the main method to 
 * perform the necessary image search operation. 
 * 
 * \param[in] mainImageFile The PNG image in which the specified searchImage 
 * is to be found and marked (for example, this will be "Flag_of_the_US.png")
 * 
 * \param[in] srchImageFile The PNG sub-image for which we will be searching
 * in the main image (for example, this will be "star.png" or "start_mask.png") 
 * 
 * \param[in] outImageFile The output file to which the mainImageFile file is 
 * written with search image file highlighted.
 * 
 * \param[in] isMask If this flag is true then the searchImageFile should 
 * be deemed as a "mask". The default value is false.
 * 
 * \param[in] matchPercent The percentage of pixels in the mainImage and
 * searchImage that must match in order for a region in the mainImage to be
 * deemed a match.
 * 
 * \param[in] tolerance The absolute acceptable difference between each color
 * channel when comparing  
 */
void imageSearch(const std::string& mainImageFile, 
    const std::string& srchImageFile, const std::string& outImageFile, 
    const bool isMask = true, const int matchPercent = 75, 
    const int tolerance = 32) {
    // Implement this method using various methods or even better
    // use an object-oriented approach.
    // Create mainIMG object and load it
    PNG mainI;
    mainI.load(mainImageFile);
    // Create maskIMG object and load it
    PNG maskI;
    maskI.load(srchImageFile);
    // Generate coordinates of each region
    vector<string> coords = genCords(maskI.getWidth(), maskI.getHeight(), 
        mainI.getWidth(), mainI.getHeight());
    // Compute the net match of each region and if the net match percent
    // is valid, add to the vector of matched regions
    vector<string> matches, potential;
    #pragma omp parallel for
    for (unsigned int i = 0; i < coords.size(); ++i) {
        int netM = getMatchCount(coords[i], tolerance, mainI, maskI);
        if (netM > maskI.getWidth() * maskI.getHeight() * matchPercent / 100) {
            #pragma omp critical
            potential.push_back(coords[i]);
        }
    }
    std::sort(potential.begin(), potential.end(), customSort);
    for (std::string match : potential) {
        addToMatches(match, matches);
    }
    // Print the matching regions
    printMatches(matches);
    // Draw the output PNG
    drawFinal(mainImageFile, maskI, matches, outImageFile);
}
    
/**
 * The main method simply checks for command-line arguments and then calls
 * the image search method in this file.
 * 
 * \param[in] argc The number of command-line arguments. This program
 * needs at least 3 command-line arguments.
 * 
 * \param[in] argv The actual command-line arguments in the following order:
 *    1. The main PNG file in which we will be searching for sub-images
 *    2. The sub-image or mask PNG file to be searched-for
 *    3. The file to which the resulting PNG image is to be written.
 *    4. Optional: Flag (True/False) to indicate if the sub-image is a mask 
 *       (deault: false)
 *    5. Optional: Number indicating required percentage of pixels to match
 *       (default is 75)
 *    6. Optiona: A tolerance value to be specified (default: 32)
 */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        // Insufficient number of required parameters.
        std::cout << "Usage: " << argv[0] << " <MainPNGfile> <SearchPNGfile> "
                  << "<OutputPNGfile> [isMaskFlag] [match-percentage] "
                  << "[tolerance]\n";
        return 1;
    }
    const std::string True("true");
    // Call the method that starts off the image search with the necessary
    // parameters.
    imageSearch(argv[1], argv[2], argv[3],       // The 3 required PNG files
        (argc > 4 ? (True == argv[4]) : true),   // Optional mask flag
        (argc > 5 ? std::stoi(argv[5]) : 75),    // Optional percentMatch
        (argc > 6 ? std::stoi(argv[6]) : 32));   // Optional tolerance
    return 0;
}
// End of source code
