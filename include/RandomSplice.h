#ifndef _RANDOM_SPLICE_
#define _RANDOM_SPLICE_

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "OutputContext.h"

#define PICK_FRAMES_RECUR_LIMIT 50

typedef struct RandSpliceParams {

    /*************** SEQUENCE PARAMS ***************/
    /*
        output filename
     */
    char output_file[1024];
    /*
        frames per second to use in sequence
     */
    double fps;
    /*
        audio sample rate
     */
    int sample_rate;

    /*************** ALGORITHM PARAMS ***************/
    /*
        directory to fetch video files
     */
    char source_dir[1024];
    /*
        duration of output video (in frames)
     */
    int64_t duration;
    /*
        average length of cuts (in frames)
     */
    int cut_len_avg;
    /*
        variability of average cuts
     */
    int cut_len_var;

    /*************** INTERNAL ONLY ***************/
    int pick_frames_recur;
} RandSpliceParams;

/**
 * Make a random edit. This function calls itself recursively to continue making
 * cuts until the output sequence is our desired length
 * @param  os  original sequence (input)
 * @param  ns  new sequence (output)
 * @param  par parameters from user to control algorithm
 * @return     0 on success (fully edited sequence), < 0 on fail
 */
int random_edit(Sequence *os, Sequence *ns, RandSpliceParams *par);

/**
 * Make a random cut from original sequence and place that clip into new sequence
 * @param  os  original sequence (input)
 * @param  ns  new sequence (output)
 * @param  par parameters from user to control algorithm
 * @return     >= 0 on success
 */
int random_cut(Sequence *os, Sequence *ns, RandSpliceParams *par);

/**
 * Cut a clip out of original sequence and insert it into new sequence in sorted order
 * @param  os          Original sequence
 * @param  ns          New Sequence
 * @param  start_index start frame of cut in original sequence
 * @param  end_index   end frame of cut in original sequence
 * @return             >= 0 on success
 */
int cut_remove_insert(Sequence *os, Sequence *ns, int start_index, int end_index);

/**
 * Randomly pick start and end frames given user parameters
 * @param  seq         Original sequence to pick frames
 * @param  par         user parameters
 * @param  start_index output index of start frame for cut
 * @param  end_index   output index of end frame for cut
 * @return             >= 0 on success
 */
int pick_frames(Sequence *seq, RandSpliceParams *par, int *start_index, int *end_index);

/**
 * Generate random number between range in the uniform distribution
 * @param  min low bound (inclusive)
 * @param  max high bound (inclusive)
 * @return     random number between min and max on a uniform distribution
 */
int rand_range(int min, int max);

/**
 * Add all files from string array into sequence!
 * @param  seq       Sequence
 * @param  files     string array of filenames (videos to be added)
 * @param  num_files number strings in files array
 * @return           >= 0 on success
 */
int add_files(Sequence *seq, char **files, int num_files);

/**
 * print string array
 * @param str  string array
 * @param rows number of strings
 */
void print_str_arr(char **str, int rows);

/**
 * Free array of strings
 * @param str  pointer to array of strings
 * @param rows number of strings in array
 */
void free_str_arr(char ***str, int rows);

/**
 * Get all filenames in directory
 * @param name of directory to search
 * @param rows number of files
 * @return array of strings (filenames)
 */
char **get_filenames_in_dir(char *dirname, int *rows);

/**
 * Tests if path is file or directory
 * @param  path filename
 * @return      >= 0 on success
 */
int is_regular_file(const char *path);

#endif