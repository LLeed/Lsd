#include "fun_head_fast.h"

const string statsName[14] = {"LENGTH", "MINI",   "MAXI",   "LQ",   "UQ",
                              "PERC05", "PERC95", "PERC50", "MEAN", "MAE",
                              "RMSE",   "L_cv",   "L_SK",   "L_KU"};

const double eps = 0.1;
const bool verbose = false;

MODELBEGIN

EQUATION("a")
RESULT(V("id_A") + T)

EQUATION("b")
RESULT(V("id_B") + T)

EQUATION("c")
RESULT(V("id_C") + T)

EQUATION("d1")
RESULT(V("id_D1") + T)

EQUATION("d2")
RESULT(V("id_D2") + T)

EQUATION("allStatsALL_P")

double expectedValues[14] = {24,      112,      343,   142,       322,
                             113,     343,      227.5, 227.5,     70.0,
                             82.4131, 0.213505, 0.0,   -0.0803361};
if (T == 1) {
  PLOG("\n Calling Stats on d1  from P at TIME= %g ", T);
  PLOG("\nTesting Correctness of eightStats function on variable D1\n");
  int error = 0;
  X_STAT_ALL("d1");
  if (verbose == true) {

    for (int i = 0; i < 14; i++) {
      PLOG("v of %d is == %g\n", i, v[i]);
      if (fabs(v[i] - expectedValues[i]) > eps) {
        error++;
        PLOG("%s ERROR! computed value from abmat is %g  \n ",
             statsName[i].c_str(), v[i]);
      }
    }
    PLOG("Number of errors in eightStats is %d \n ", error);
  }
  else {
    PLOG("Number of errors in eightStats is %d\n", error);
  }
}
RESULT(0)

EQUATION("StatsA")

double expectedValues[3][3] = {{8, 112, 143}, {8, 212, 243}, {8, 312, 343}};
double ID = V("id_A");
int error = 0;
if (T == 1 && ID <= 3) {
  PLOG("\n Testing Correct data gathering\n");
  PLOG("Call Stats on D1 from Object A with ID %g\n", ID);
  X_STAT_ALL("d1");
  for (int i = 0; i < 3; i++) {
    j = ID - 1;
    if (v[i] != expectedValues[j][i]) {
      PLOG("Input Data Error at %s \n", statsName[i].c_str());
      error++;
      // break;
    }
  }
  PLOG("ERROR = %d \n", error);
}

if (T == 9) {
  PLOG("\n Testing time series data gathering of variable a ");
  if (verbose == true)
    PLOG("of Object A with"
         "ID=%g \n",
         V("id_A"));
  double expectedValues[3][3] = {{9, 2, 10}, {9, 3, 11}, {9, 4, 12}};
  T_STAT("a");
  for (int i = 0; i < 3; i++) {
    j = ID - 1;
    if (v[i] != expectedValues[j][i]) {
      if (verbose == true)
        PLOG("Input Data Error at %s \n", statsName[i].c_str());
      error++;
      // break;
    }
  }
  PLOG("ERROR = %d \n", error);
  if (V("id_A") == 1) {
    T_STAT_INTVL("a", 1);
    if (v[2] == 9) {
      PLOG("T_STAT_INTVL OK \n");
    }
    else
      PLOG("T_STAT_INTVL ERROR \n");
  }
}

RESULT(0)

MODELEND

void close_sim(void)
{
  // close simulation special commands go here
}
