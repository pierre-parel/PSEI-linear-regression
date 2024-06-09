#include <ctime>
#include <eigen3/Eigen/Dense>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace Eigen;

void plotData() {
  FILE *gnuplotPipe = popen("gnuplot -persist", "w");
  if (!gnuplotPipe) {
    cerr << "Error opening Gnuplot pipe!" << endl;
    return;
  }

  fprintf(gnuplotPipe, "set terminal pngcairo enhanced font 'Verdana,12'\n");
  fprintf(gnuplotPipe, "set output 'predicted_vs_actual_open.png'\n");
  fprintf(gnuplotPipe, "set datafile separator ','\n");
  fprintf(gnuplotPipe, "set xdata time\n");
  fprintf(gnuplotPipe, "set timefmt \"%%m/%%d/%%Y\"\n");
  fprintf(gnuplotPipe, "set format x \"%%m/%%d\"\n");
  fprintf(gnuplotPipe, "set title \"Predicted vs Actual Open Over Time\"\n");
  fprintf(gnuplotPipe, "set xlabel \"Date\"\n");
  fprintf(gnuplotPipe, "set ylabel \"Open Price\"\n");
  fprintf(gnuplotPipe, "plot 'open_data.csv' using 1:2 with lines title"
                       "'Predicted Open' lc rgb 'blue' ,\\\n");
  fprintf(gnuplotPipe, "  'open_data.csv' using 1:3 with lines title "
                       "'Actual Open' lc rgb 'red',\\\n");
  pclose(gnuplotPipe);

  cout << "Plot generated: predicted_vs_actual_open.png" << endl;
}

void writeDataToCSV(const vector<string> &dates,
                    const vector<double> &predictedOpen,
                    const vector<double> &actualOpen, const string &filename) {
  ofstream dataFile(filename);
  if (!dataFile.is_open()) {
    cerr << "Error opening file " << filename << " for writing!" << endl;
    return;
  }

  dataFile << "Date,PredictedOpen,ActualOpen" << endl;

  for (size_t i = 0; i < dates.size(); ++i) {
    dataFile << dates[i] << "," << predictedOpen[i] << "," << actualOpen[i]
             << endl;
  }

  dataFile.close();

  cout << "Data written to " << filename << " successfully." << endl;
}

// Convert Excel date to a numerical value
double excelTime(const string &date, const string &baseDate) {
  int year, month, day;
  sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day);

  int baseYear, baseMonth, baseDay;
  sscanf(baseDate.c_str(), "%d-%d-%d", &baseYear, &baseMonth, &baseDay);

  tm dateTm = {0, 0, 0, day, month - 1, year - 1900};
  tm baseTm = {0, 0, 0, baseDay, baseMonth - 1, baseYear - 1900};

  time_t dateT = mktime(&dateTm);
  time_t baseT = mktime(&baseTm);

  return difftime(dateT, baseT) / (60 * 60 * 24);
}

// Function to convert Excel serial number to MM/DD/YYYY format
string excelSerialToDate(double serial) {
  time_t t = (serial - 25569) * 86400; // 25569 is the difference between
                                       // 1970-01-01 and 1900-01-01 in days
  tm *timePtr = gmtime(&t);

  char buffer[11];
  strftime(buffer, 11, "%m/%d/%Y", timePtr);
  return string(buffer);
}

int main() {
  ifstream file("./PSEI.csv");
  string line;

  vector<double> dates, opens;
  string baseDate = "1900-01-01";
  getline(file, line); // Skips first line (header of csv)
  while (getline(file, line)) {
    stringstream ss(line);
    string date, openStr;
    for (int i = 0; i < 5; ++i) {
      if (i == 0)
        getline(ss, date, ',');
      else
        getline(ss, openStr, ',');
    }
    dates.push_back(excelTime(date, baseDate));
    if (openStr != "null")
      opens.push_back(stod(openStr));
  }

  int n = opens.size();
  VectorXd x(n), y(n);

  for (int i = 0; i < n; ++i) {
    x(i) = dates[i];
    y(i) = opens[i];
  }

  double x_sum = x.sum();
  double y_sum = y.sum();
  double x2_sum = x.dot(x);
  double xy_sum = x.dot(y);

  double x_ave = x_sum / n;
  double y_ave = y_sum / n;

  double m =
      ((n * xy_sum) - (x_sum * y_sum)) / ((n * x2_sum) - (x_sum * x_sum));
  double b = y_ave - (m * x_ave);

  VectorXd predictDates(30);
  for (int i = 0; i < 30; ++i) {
    predictDates(i) = 45413 + i;
  }
  // vector<int> predictedDates(predictDates.data(),
  //                         predictDates.data() + predictDates.size());
  vector<string> predictedDates;
  for (int day = 1; day <= 30; ++day) {
    ostringstream dateStream;
    dateStream << setfill('0') << setw(2) << 5 << "/" << setw(2) << day << "/"
               << 2024;
    predictedDates.push_back(dateStream.str());
  }

  VectorXd predictOpen = (m * predictDates.array()).matrix() +
                         VectorXd::Ones(predictDates.size()) * b;
  vector<double> predictedOpen(predictOpen.data(),
                               predictOpen.data() + predictOpen.size());

  for (int i = 0; i < predictedOpen.size(); ++i) {
    string dateStr = excelSerialToDate(predictDates(i));
    cout << "Predicted open on " << dateStr << " is " << predictedOpen[i]
         << endl;
  }

  vector<double> actualOpen = {
      6654.97, 6654.97, 6664.49, 6664.49, 6664.49, 6639.27, 6668.24, 6620.35,
      6670.85, 6574.22, 6574.22, 6574.22, 6523.88, 6601.93, 6578.65, 6589.24,
      6619.51, 6619.51, 6619.51, 6633.59, 6704.14, 6606.80, 6612.09, 6634.61,
      6634.61, 6634.61, 6590.97, 6572.96, 6506.40, 6506.40, 6506.40};

  writeDataToCSV(predictedDates, predictedOpen, actualOpen, "open_data.csv");

  plotData();

  return 0;
}
