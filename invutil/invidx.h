#ifndef INVIDX_H
#define INVIDX_H

#include "types.h"
#include "mlog.h"
#include "inv_priv.h"
#include "list.h"
#include "cmenu.h"

/* invidx.c */
node_t *generate_invidx_menu(char *inv_path, node_t *startnode, int level, char *idxFileName);
int open_invidx(char *idxFileName);
int close_invidx(int idx);
int close_all_invidx(void);
void *remmap_invidx(int fidx, int num);
int find_matching_stobjfile(char *filename);
int find_matching_invidxfile(char *filename);
int find_overlapping_invidx(int fidx, invt_entry_t *inv_entry);
int find_invidx_insert_point(int fd, time_t sh_time);
int find_invidx_insert_pos(int fidx, invt_entry_t *inv_entry);
char *get_inventry_stobjfile(data_t *d);
node_t *find_invidx_node(node_t *list, int inv_fileidx);
int read_stobj_info(int fd, int idx, invt_seshdr_t **hdr, invt_session_t **ses,
		invt_stream_t **strms, invt_mediafile_t **mfiles);
int insert_stobj_into_stobjfile(int invidx_fileidx, char *filename, int fd,
 		invt_seshdr_t *hdr, invt_session_t *ses, invt_stream_t *strms,
		invt_mediafile_t *mfiles);
int insert_stobj_into_inventory(int invidx_fileidx, node_t *node);
int invidx_undelete(WINDOW *win, node_t *current, node_t *list);
int invidx_select(WINDOW *win, node_t *current, node_t *list);
int invidx_highlight(WINDOW *win, node_t *current, node_t *list);
int invidx_commit(WINDOW *win, node_t *current, node_t *list);
int invidx_prune(char *mountpt, uuid_t *uuidp, time_t prunetime, node_t *node, node_t *list);

#endif /* INVIDX_H */