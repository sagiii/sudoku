#include <iostream>
#include <algorithm>
#include <vector>
#include <unistd.h>

using namespace std;

bool verbose = false;
bool slow = false;
typedef enum {
  MODE_SILENT,
  MODE_VERBOSE,
  MODE_VERBOSE_SLOW,
  MODE_UNDEFINED,
} MODE_T;
MODE_T mode = MODE_SILENT;
void apply_mode(MODE_T mode)
{
  switch(mode) {
    case MODE_VERBOSE:
    case MODE_VERBOSE_SLOW:
    verbose = true;
    break;
    default:
    verbose = false;
    break;
  }
  switch(mode) {
    case MODE_VERBOSE_SLOW:
    slow = true;
    break;
    default:
    slow = false;
    break;
  }
}

typedef struct {
  int r;
  int c;
  int n;
  int same_rc;
} assm_t; // assumption element

bool compare_assms(const assm_t &a, const assm_t &b)
{
  return (a.same_rc < b.same_rc);
}

void parse(istream &in, int map[9][9])
{
  for (int i = 0; i < 9; i ++ ) {
    for (int j = 0; j < 9; j ++ ) {
      in >> map[i][j];
    }
  }
}

void print(ostream &out, int map[9][9], bool sep = true)
{
  if (sep) {
    out << "--------------------" << endl;
  }
  for (int i = 0; i < 9; i ++ ) {
    for (int j = 0; j < 9; j ++ ) {
      out << map[i][j] << " ";
    }
    out << endl;
  }
  out.flush();
}

void get_block_origin(int r, int c, int &r0, int &c0)
{
  r0 = (r / 3) * 3;
  c0 = (c / 3) * 3;
}

void get_groups(int r, int c, int groups[3][8][2])
{
  // same row
  {
    int cnt = 0;
    for (int c1 = 0; c1 < 9; c1 ++) {
      if (c != c1) {
        int p[] = {r, c1};
        groups[0][cnt][0] = r;
        groups[0][cnt][1] = c1;
        cnt++;
      }
    }
  }
  // same column
  {
    int cnt = 0;
    for (int r1 = 0; r1 < 9; r1 ++) {
      if (r != r1) {
        groups[1][cnt][0] = r1;
        groups[1][cnt][1] = c;
        cnt++;
      }
    }
  }
  // same block
  {
    int r0, c0;
    get_block_origin(r, c, r0, c0);
    int cnt = 0;
    for (int r1 = r0; r1 < r0 + 3; r1 ++) {
      for (int c1 = c0; c1 < c0 + 3; c1 ++) {
        if (r != r1 || c != c1) {
          groups[2][cnt][0] = r1;
          groups[2][cnt][1] = c1;
          cnt++;
        }
      }
    }  
  }
}

void update_possible(int det[9][9], bool pos[9][9][9])
{
  // row
  for (int r = 0; r < 9; r ++ ) {
    // column
    for (int c = 0; c < 9; c ++ ) {
      // check if determined
      if (det[r][c] > 0) {
        for (int n = 0; n < 9; n ++ ) {
          if (n == det[r][c] - 1) {
            pos[r][c][n] = true;
          } else {
            pos[r][c][n] = false;
          }
        }
      } else {
        // scan for number (-1)
        for (int n = 0; n < 9; n ++ ) {
          pos[r][c][n] = true; // clear
          int number = n + 1;
          int groups[3][8][2];
          get_groups(r, c, groups);
          for (int gi = 0; gi < 3; gi ++) {
            for (int mi = 0; mi < 8; mi ++) {
              int r1 = groups[gi][mi][0], c1 = groups[gi][mi][1];
              if (det[r1][c1] == number) {
                pos[r][c][n] = false;
                goto next_number;
              }
            }
          }
          next_number:;
        }
      }
    }
  }  
}

void update_determined(bool pos[9][9][9], int det[9][9])
{
  // row
  for (int r = 0; r < 9; r ++ ) {
    // column
    for (int c = 0; c < 9; c ++ ) {
      det[r][c] = 0; // clear
      // scan for number at the box
      {
        int candidate, count = 0;
        // number (-1)
        for (int n = 0; n < 9; n ++ ) {
          int number = n + 1;
          if (pos[r][c][n]) {
            candidate = number;
            count ++;
          }
        }
        if (count == 1) {
          det[r][c] = candidate;
          goto next_box;
        }
      }
      for (int n = 0; n < 9; n ++ ) {
        int groups[3][8][2];
        get_groups(r, c, groups);
        for (int gi = 0; gi < 3; gi ++) {
          int cnt = 0;
          for (int mi = 0; mi < 8; mi ++) {
            int r1 = groups[gi][mi][0], c1 = groups[gi][mi][1];
            if (pos[r1][c1][n]) {
              cnt ++;
            }
          }
          if (cnt == 0) {
            det[r][c] = n + 1;
            goto next_box;
          }
        }
      }
      next_box:;
    }
  }
}

void list_assumptions(bool pos[9][9][9], vector<assm_t> &assms)
{
  assms.clear();
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 9; c++) {
      int cnt = 0;
      for (int n = 0; n < 9; n ++ ) {
        if (pos[r][c][n]) cnt++;
      }
      if (cnt > 1) {
        for (int n = 0; n < 9; n ++ ) {
          if (pos[r][c][n]) {
            assm_t assm = {r, c, n, cnt};
            assms.push_back(assm);
          }
        }
      }
    }
  }
  // sort by same_rc value in ascending order.
  sort(assms.begin(), assms.end(), compare_assms);
}

bool is_completed(int determined[9][9])
{
  // check if there's no undetermined
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 9; c++) {
      if (determined[r][c] == 0) return false;
    }
  }
  // check if there's no duplicates
  {
    // row groups
    for (int r = 0; r < 9; r ++) {
      int count[9] = {0};
      for (int c = 0; c < 9; c ++) {
        count[determined[r][c] - 1] ++;
      }
      for (int n = 0; n < 9; n ++) {
        if (count[n] != 1) return false;
      }
    }
    // column groups
    for (int c = 0; c < 9; c ++) {
      int count[9] = {0};
      for (int r = 0; r < 9; r ++) {
        count[determined[r][c] - 1] ++;
      }
      for (int n = 0; n < 9; n ++) {
        if (count[n] != 1) return false;
      }
    }
    // block groups
    for (int c0 = 0; c0 < 9; c0 += 3) {
      for (int r0 = 0; r0 < 9; r0 += 3) {
        int count[9] = {0};
        for (int r1 = r0; r1 < r0 + 3; r1 ++) {
          for (int c1 = c0; c1 < c0 + 3; c1 ++) {
            count[determined[r1][c1] - 1] ++;
          }
        }
        for (int n = 0; n < 9; n ++) {
          if (count[n] != 1) return false;
        }
      }
    }
  }
  return true;
}

bool is_completed(bool pos[9][9][9])
{
  for (int r = 0; r < 9; r ++) {
    for (int c = 0; c < 9; c ++) {
      int count = 0;
      for (int n = 0; n < 9; n ++) {
        if (pos[r][c][n]) count ++;
      }
      if (count != 1) return false;
    }
  }
  return true;
}

bool is_valid(bool pos[9][9][9])
{
  // check if there's no cell without possbility
  for (int r = 0; r < 9; r ++) {
    for (int c = 0; c < 9; c ++) {
      int count = 0;
      for (int n = 0; n < 9; n ++) {
        if (pos[r][c][n]) count ++;
      }
      if (count == 0) return false;
    }
  }
  // check if there's no group without possbility
  for (int n = 0; n < 9; n ++ ) {
    int number = n + 1;
    // row groups
    for (int r = 0; r < 9; r ++) {
      int count = 0;
      for (int c = 0; c < 9; c ++) {
        if (pos[r][c][n]) count ++;
      }
      if (count == 0) return false;
    }
    // column groups
    for (int c = 0; c < 9; c ++) {
      int count = 0;
      for (int r = 0; r < 9; r ++) {
        if (pos[r][c][n]) count ++;
      }
      if (count == 0) return false;
    }
    // block groups
    for (int c0 = 0; c0 < 9; c0 += 3) {
      for (int r0 = 0; r0 < 9; r0 += 3) {
        int count = 0;
        for (int r1 = r0; r1 < r0 + 3; r1 ++) {
          for (int c1 = c0; c1 < c0 + 3; c1 ++) {
            if (pos[r1][c1][n]) count ++;
          }
        }
        if (count == 0) return false;
      }
    }
  }
  return true;
}

// return if stuck point is valid
bool proceed_till_stuck(bool pos_from[9][9][9], bool pos_to[9][9][9])
{
  int det0[9][9], det1[9][9]; // 0 means not determined.
  bool pos[9][9][9];
  memcpy(pos, pos_from, sizeof(pos));
  update_determined(pos, det0);
  if (verbose) {
    print(cout, det0);
  }
  if (slow) {
    usleep(500000);
  }
  while (true) {
    memcpy(det1, det0, sizeof(det0));
    update_possible(det0, pos);
    if (!is_valid(pos)) {
      if (verbose)
        cout << "Invalid." << endl;
      return false;
    }
    update_determined(pos, det0);
    if (verbose) {
      print(cout, det0);
    }
    if (slow) {
      usleep(500000);
    }
    if (is_completed(det0)) {
      cout << "Completed." << endl;
      print(cout, det0);
      exit(0);
      break;
    }
    // stuck!
    if (memcmp(det0, det1, sizeof(det0)) == 0) {
      if (verbose)
        cout << "I'm stuck." << endl;
      break;
    }
  }
  update_possible(det0, pos_to);
  return true;
}

// return if solution is found
bool explore(bool pos[9][9][9])
{
  bool pos1[9][9][9];
  if (proceed_till_stuck(pos, pos1)) {
    vector<assm_t> assms;
    list_assumptions(pos1, assms);
    for (vector<assm_t>::iterator i = assms.begin(); i != assms.end(); i++) {
      bool pos2[9][9][9];
      memcpy(pos2, pos1, sizeof(pos1));
      char desc[100];
      sprintf(desc, "(%d,%d) != %d", i->r, i->c, i->n);
      if (verbose)
        cout << "Assume " << desc << "." << endl;
      pos2[i->r][i->c][i->n] = false;
      if (explore(pos2)) {
        return true;
      } else {
        if (verbose)
          cout << "Discard " << desc << "." << endl;
      }
    }
  } else {
    // return immediately from invalid branch
    return false;
  }
  return false;
}

int main(int argc, char **argv)
{
  if (argc > 1) {
    int m;
    sscanf(argv[1], "%d", &m);
    if (m < MODE_UNDEFINED) {
      mode = (MODE_T)m;
    }
  }
  apply_mode(mode);
  int determined[9][9]; // 0 means not determined.
  bool possible[9][9][9];
  parse(cin, determined);
  cout << "input:" << endl;
  print(cout, determined);
  update_possible(determined, possible);
  explore(possible);
  return 0;
}
