#include "RandomSplice.h"

/**
 * valgrind --leak-check=yes bin/examples/random-splice test-resources/sequence/out7.mov 30 48000 test-resources/sequence/ 300 80 40
 */
int main(int argc, char **argv) {
    if(argc < 8) {
        printf("usage: %s output_file fps sample_rate source_dir duration cut_len_avg cut_len_var\n", argv[0]);
        printf("\nExplanation\n------------\n");
        printf("output_file(string): output filename of encoded edit (ex. out.mov)\n");
        printf("fps(int): frames per second to use in sequence. All frame parameters are based on this (ex. 30 for 30fps)\n");
        printf("sample_rate(int): audio sample rate (ex. 48000 for 48kHz)\n");
        printf("source_dir(string): directory of raw video files that will be used in the edit\n");
        printf("duration(int): duration of output file (in frames - fps defined above)\n");
        printf("cut_len_avg(int): average length of cuts (in frames)\n");
        printf("cut_len_var(int): variability of average cuts used by the random number generator for max and min range\n");
        return -1;
    }
    RandSpliceParams par;
    strcpy(par.output_file, argv[1]);
    sscanf(argv[2], "%lf", &(par.fps));
    par.sample_rate = atoi(argv[3]);
    strcpy(par.source_dir, argv[4]);
    par.duration = atoi(argv[5]);
    par.cut_len_avg = atoi(argv[6]);
    par.cut_len_var = atoi(argv[7]);
    par.pick_frames_recur = 0;

    int num_files, ret = 0;
    char **files = get_filenames_in_dir(par.source_dir, &num_files);
    srand(time(NULL));

    Sequence orig_seq, new_seq;
    init_sequence_cmp(&orig_seq, par.fps, par.sample_rate, &list_compare_clips_sequential);
    init_sequence_cmp(&new_seq, par.fps, par.sample_rate, &list_compare_clips_sequential);

    ret = add_files(&orig_seq, files, num_files);
    if(ret < 0) {
        fprintf(stderr, "failed to add files\n");
        goto end;
    }

    char *str = print_sequence(&orig_seq);
    printf("====== ORIG_SEQ =====\n%s\n", str);
    free(str);
    str = NULL;

    ret = random_edit(&orig_seq, &new_seq, &par);
    if(ret < 0) {
        fprintf(stderr, "random_edit() error: Failed to finish edit\n");
        goto end;
    }

    str = print_sequence(&new_seq);
    printf("====== NEW_SEQ =====\n%s\n", str);
    free(str);
    str = NULL;

    if(new_seq.clips.head != NULL) {
        // output parameters
        OutputParameters op;
        VideoOutParams vp;
        AudioOutParams ap;
        Clip *clip1 = (Clip *) (new_seq.clips.head->data);
        set_video_out_params(&vp, clip1->vid_ctx->video_codec_ctx);
        vp.codec_id = AV_CODEC_ID_NONE;
        vp.bit_rate = -1;
        set_audio_out_params(&ap, clip1->vid_ctx->audio_codec_ctx);
        if(set_output_params(&op, par.output_file, vp, ap) < 0) {
            fprintf(stderr, "Failed to set output params\n");
            goto end;
        }

        ret = write_sequence(&new_seq, &op);
        free_output_params(&op);
        if(ret < 0) {
            fprintf(stderr, "Failed to write new sequence to output file[%s]\n", op.filename);
            goto end;
        }
    }

    end:
    // print_str_arr(files, num_files);
    free_str_arr(&files, num_files);
    free_sequence(&orig_seq);
    free_sequence(&new_seq);
}

/**
 * Make a random edit. This function calls itself recursively to continue making
 * cuts until the output sequence is our desired length
 * @param  os  original sequence (input)
 * @param  ns  new sequence (output)
 * @param  par parameters from user to control algorithm
 * @return     0 on success (fully edited sequence), < 0 on fail
 */
int random_edit(Sequence *os, Sequence *ns, RandSpliceParams *par) {
    if(par->cut_len_var > par->cut_len_avg) {
        fprintf(stderr, "random_edit() error: cut_len_var[%d] must be less than cut_len_avg[%d]\n", par->cut_len_var, par->cut_len_avg);
        return -1;
    }
    if(get_sequence_duration(ns) > par->duration) {
        return 0;
    }
    int ret = random_cut(os, ns, par);
    if(ret < 0) {
        return ret;
    }
    return random_edit(os, ns, par);
}

/**
 * Make a random cut from original sequence and place that clip into new sequence
 * @param  os  original sequence (input)
 * @param  ns  new sequence (output)
 * @param  par parameters from user to control algorithm
 * @return     >= 0 on success
 */
int random_cut(Sequence *os, Sequence *ns, RandSpliceParams *par) {
    int s, e;
    int ret = pick_frames(os, par, &s, &e);
    if(ret < 0) {
        fprintf(stderr, "random_cut() error: Failed to pick frames\n");
        return ret;
    }
    return cut_remove_insert(os, ns, s, e);
}

/**
 * Cut a clip out of original sequence and insert it into new sequence in sorted order
 * @param  os          Original sequence
 * @param  ns          New Sequence
 * @param  start_index start frame of cut in original sequence
 * @param  end_index   end frame of cut in original sequence
 * @return             >= 0 on success
 */
int cut_remove_insert(Sequence *os, Sequence *ns, int start_index, int end_index) {
    int cut_center_index = start_index + ((end_index - start_index) / 2);
    int ret = cut_clip(os, start_index);
    if(ret < 0) {
        fprintf(stderr, "cut_remove_insert() error: Failed to cut clip at start index[%d]\n", start_index);
        return ret;
    }
    ret = cut_clip(os, end_index);
    if (ret < 0) {
        fprintf(stderr, "cut_remove_insert() error: Failed to cut clip at end index[%d]\n", end_index);
        return ret;
    }
    Clip *cut = NULL;
    find_clip_at_index(os, cut_center_index, &cut);
    if(!cut) {
        fprintf(stderr, "cut_remove_insert() error: Failed to find clip at cut center index[%d]\n", cut_center_index);
        return -1;
    }
    Clip *cut_copy = copy_clip_vc(cut);
    if(cut_copy == NULL) {
        fprintf(stderr, "cut_remove_insert() error: Failed to allocate cut_copy\n");
        return -1;
    }
    ret = set_clip_bounds_pts(cut_copy, cut->orig_start_pts, cut->orig_end_pts);
    if(ret < 0) {
        free_clip(&cut_copy);
        fprintf(stderr, "cut_remove_insert() error: Failed to set clip bounds on cut copy\n");
        return ret;
    }
    ret = sequence_insert_clip_sorted(ns, cut_copy);
    if(ret < 0) {
        free_clip(&cut_copy);
        fprintf(stderr, "cut_remove_insert() error: failed to add clip[%s] to new sequence\n", cut_copy->vid_ctx->url);
        return ret;
    }
    ret = sequence_ripple_delete_clip(os, cut);
    if(ret < 0) {
        free_clip(&cut_copy);
        fprintf(stderr, "cut_remove_insert() error: Failed to delete clip from original sequence\n");
        return ret;
    }
    return 0;
}

/**
 * Randomly pick start and end frames given user parameters
 * @param  seq         Original sequence to pick frames
 * @param  par         user parameters
 * @param  start_index output index of start frame for cut
 * @param  end_index   output index of end frame for cut
 * @return             >= 0 on success
 */
int pick_frames(Sequence *seq, RandSpliceParams *par, int *start_index, int *end_index) {
    if(par->pick_frames_recur > PICK_FRAMES_RECUR_LIMIT) {
        fprintf(stderr, "pick_frames() error: par->pick_frames_recur[%d] > limit[%d]\nThis should never happen\n", par->pick_frames_recur, PICK_FRAMES_RECUR_LIMIT);
        return -1;
    }
    int64_t seq_dur = get_sequence_duration(seq);
    if(seq_dur <= 0) {
        fprintf(stderr, "pick_frames() error: sequence duration[%ld] is invalid\n", seq_dur);
        return -1;
    }
    int s = rand_range(0, seq_dur - par->cut_len_avg - 1);
    int e_var;
    if(par->cut_len_var == 0) {
        e_var = 0;
    } else {
        e_var = rand_range((-1)*(par->cut_len_var), par->cut_len_var);
    }
    int e = s + par->cut_len_avg + e_var;
    if(e > seq_dur) {
        e = seq_dur;
    }
    Clip *sc = NULL, *se = NULL;
    find_clip_at_index(seq, s, &sc);
    find_clip_at_index(seq, e, &se);
    if(!sc || !se) {
        fprintf(stderr, "pick_frames() error: clip does not exist at start[%d] or end[%d] index\n", s, e);
        return -1;
    }
    // If start and end index do not lie on the same clip.. try again
    if(compare_clips_sequential(sc, se) != 0) {
        ++(par->pick_frames_recur);
        return pick_frames(seq, par, start_index, end_index);
    }
    *start_index = s;
    *end_index = e;
    par->pick_frames_recur = 0;
    return 0;
}

/**
 * Generate random number between range in the uniform distribution
 * @param  min low bound (inclusive)
 * @param  max high bound (inclusive)
 * @return     random number between min and max on a uniform distribution
 */
int rand_range(int min, int max) {
    int diff = max-min;
    return (int) (((double)(diff+1)/RAND_MAX) * rand() + min);
}

/**
 * Add all files from string array into sequence!
 * @param  seq       Sequence
 * @param  files     string array of filenames (videos to be added)
 * @param  num_files number strings in files array
 * @return           >= 0 on success
 */
int add_files(Sequence *seq, char **files, int num_files) {
    int ret;
    for(int i = 0; i < num_files; i++) {
        Clip *curr = alloc_clip(files[i]);
        if(curr == NULL) {
            printf("add_files() warning: failed to allocate clip[%s]\n", files[i]);
        }
        ret = sequence_insert_clip_sorted(seq, curr);
        if(ret < 0) {
            fprintf(stderr, "add_files() error: failed to add clip[%s] to sequence\n", files[i]);
            return ret;
        }
        printf("added clip[%s] to original sequence\n", curr->vid_ctx->url);
    }
    return 0;
}

/**
 * print string array
 * @param str  string array
 * @param rows number of strings
 */
void print_str_arr(char **str, int rows) {
    for (int i = 0; i < rows; i++) {
        printf("%s\n", str[i]);
    }
}

/**
 * Free array of strings
 * @param str  pointer to array of strings
 * @param rows number of strings in array
 */
void free_str_arr(char ***str, int rows) {
    char **s = *str;
    if(s != NULL) {
        for(int i = 0; i < rows; i++) {
            free(s[i]);
            s[i] = NULL;
        }
        free(*str);
        *str = NULL;
    }
}

/**
 * Get all filenames in directory
 * @param name of directory to search
 * @param rows number of files
 * @return array of strings (filenames)
 */
char **get_filenames_in_dir(char *dirname, int *rows) {
    char buf[4096];
    buf[0] = 0;
    *rows = 0;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dirname)) != NULL) {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
          strcat(buf, dirname);
          strcat(buf, ent->d_name);
          if(!is_regular_file(buf)) {
              buf[0] = 0;
              continue;
          }
          buf[0] = 0;
          ++(*rows);
      }
    } else {
      perror ("");
      return NULL;
    }
    rewinddir(dir);

    char **str = malloc(sizeof(char *) * (*rows));

    /* print all the files and directories within directory */
    int i = 0;
    while ((ent = readdir (dir)) != NULL) {
        strcat(buf, dirname);
        strcat(buf, ent->d_name);
        if(!is_regular_file(buf)) {
            buf[0] = 0;
            continue;
        }
        str[i] = malloc(sizeof(char) * (strlen(buf) + 1));
        strcpy(str[i++], buf);
        buf[0] = 0;
    }
    closedir (dir);
    return str;
}

/**
 * Tests if path is file or directory
 * @param  path filename
 * @return      >= 0 on success
 */
int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}