/* midmerge.c
 * Take a directory of MIDI files and collapse all tracks from each into one channel.
 * Emit the whole thing as one track.
 */

#include "tool_internal.h"

/* Context.
 */
 
struct midmerge_context {
  struct infile {
    char *path;
    uint8_t *src;
    int srcc;
    int chid;
    int division;
    struct track {
      const uint8_t *v;
      int c;
      int p;
      int delay;
      int status;
    } *trackv;
    int trackc,tracka;
  } *filev;
  int filec,filea;
  struct sr_encoder dst;
  int wtime; // Total ticks at last written event.
  int now; // Running total ticks.
};

static void infile_cleanup(struct infile *file) {
  if (file->path) free(file->path);
  if (file->src) free(file->src);
  if (file->trackv) free(file->trackv); // track cleanup not required
}

static void midmerge_context_cleanup(struct midmerge_context *ctx) {
  if (ctx->filev) {
    while (ctx->filec-->0) infile_cleanup(ctx->filev+ctx->filec);
    free(ctx->filev);
  }
  sr_encoder_cleanup(&ctx->dst);
}

/* Choose channel based on path of input single-channel file.
 * Returns >=16 to explicitly ignore the file, <0 on real errors, or a MIDI chid.
 */
 
static int midmerge_chid_by_path(const char *path) {
  // Split basename on dots and underscores.
  const char *base=path;
  int basec=0,pathp=0;
  for (;path[pathp];pathp++) {
    if (path[pathp]=='/') {
      base=path+pathp+1;
      basec=0;
    } else basec++;
  }
  int basep=0;
  while (basep<basec) {
    const char *token=base+basep;
    int tokenc=0;
    while ((basep<basec)&&(base[basep]!='_')&&(base[basep]!='.')) { tokenc++; basep++; }
    basep++;
    
    if ((tokenc==4)&&!memcmp(token,"Bass",4)) return 0;
    if ((tokenc==4)&&!memcmp(token,"Lead",4)) return 1;
    if ((tokenc==5)&&!memcmp(token,"Piano",5)) return 2;
    if ((tokenc==5)&&!memcmp(token,"Drums",5)) return 3;
    if ((tokenc==3)&&!memcmp(token,"Pad",3)) return 4;
  }
  return -1;
}

/* Add track to file.
 */
 
static int infile_add_track(struct infile *file,const uint8_t *v,int c) {
  if (file->trackc>=file->tracka) {
    int na=file->tracka+8;
    if (na>INT_MAX/sizeof(struct track)) return -1;
    void *nv=realloc(file->trackv,sizeof(struct track)*na);
    if (!nv) return -1;
    file->trackv=nv;
    file->tracka=na;
  }
  struct track *track=file->trackv+file->trackc++;
  memset(track,0,sizeof(struct track));
  track->v=v;
  track->c=c;
  track->delay=-1;
  return 0;
}

/* Initial read of file.
 * Populates (division,trackv).
 */
 
static int infile_read_toc(struct midmerge_context *ctx,struct infile *file) {
  file->division=0;
  file->trackc=0;
  int srcp=0;
  while (srcp<file->srcc) {
    
    if (srcp>file->srcc-8) {
      fprintf(stderr,"%s: Unexpected EOF\n",file->path);
      return -1;
    }
    const char *chunkid=file->src+srcp;
    srcp+=4;
    int chunklen=file->src[srcp++]<<24;
    chunklen|=file->src[srcp++]<<16;
    chunklen|=file->src[srcp++]<<8;
    chunklen|=file->src[srcp++];
    if ((chunklen<0)||(srcp>file->srcc-chunklen)) {
      fprintf(stderr,"%s: Invalid chunk length\n",file->path);
      return -1;
    }
    const uint8_t *chunk=file->src+srcp;
    srcp+=chunklen;
    
    if (!memcmp(chunkid,"MThd",4)) {
      if (chunklen<6) {
        fprintf(stderr,"%s: MThd too small\n",file->path);
        return -1;
      }
      file->division=(chunk[4]<<8)|chunk[5];
      if ((file->division<1)||(file->division>=0x8000)) {
        fprintf(stderr,"%s: Invalid division 0x%04x\n",file->path,file->division);
        return -1;
      }
    } else if (!memcmp(chunkid,"MTrk",4)) {
      if (infile_add_track(file,chunk,chunklen)<0) return -1;
    }
  }
  if (!file->division) {
    fprintf(stderr,"%s: No MThd\n",file->path);
    return -1;
  }
  return 0;
}

/* Add file to context.
 */
 
static struct infile *midmerge_add_file(struct midmerge_context *ctx,const char *path) {
  if (ctx->filec>=ctx->filea) {
    int na=ctx->filea+16;
    if (na>INT_MAX/sizeof(struct infile)) return 0;
    void *nv=realloc(ctx->filev,sizeof(struct infile)*na);
    if (!nv) return 0;
    ctx->filev=nv;
    ctx->filea=na;
  }
  struct infile *file=ctx->filev+ctx->filec++;
  memset(file,0,sizeof(struct infile));
  if (!(file->path=strdup(path))) { ctx->filec--; return 0; }
  if ((file->srcc=file_read(&file->src,path))<0) {
    ctx->filec--;
    infile_cleanup(file);
    return 0;
  }
  return file;
}

/* Receive input file.
 */
 
static int cb_midmerge_input(const char *path,const char *base,char ftype,void *userdata) {
  struct midmerge_context *ctx=userdata;
  if (!ftype) ftype=file_get_type(path);
  if (ftype!='f') return 0;
  
  int chid=midmerge_chid_by_path(path);
  if (chid<0) {
    fprintf(stderr,"%s: Unexpected name. Please update midmerge_chid_by_path()\n",path);
    return -1;
  }
  if (chid>=16) return 0;
  
  struct infile *file=midmerge_add_file(ctx,path);
  if (!file) return -1;
  
  file->chid=chid;
  if (infile_read_toc(ctx,file)<0) {
    fprintf(stderr,"%s: Error reading MIDI file initially\n",path);
    return -1;
  }
  return 0;
}

/* Write one VLQ delay to output.
 * Caller must follow by writing an event.
 * Updates (ctx->wtime) to equal (ctx->now).
 */
 
static int midmerge_write_delay(struct midmerge_context *ctx) {
  int tickc=ctx->now-ctx->wtime;
  ctx->wtime=ctx->now;
  if (tickc<0) tickc=0; else if (tickc>0x0fffffff) tickc=0x0fffffff;
  if (sr_encode_vlq(&ctx->dst,tickc)<0) return -1;
  return 0;
}

/* Acquire delay for a track.
 */
 
static int track_read_delay(struct track *track) {
  int len=sr_vlq_decode(&track->delay,track->v+track->p,track->c-track->p);
  if (len<0) return -1;
  track->p+=len;
  return 0;
}

/* Read and process one event off (track).
 * Must not be a delay.
 * May produce output.
 */
 
static int midmerge_process_event(struct midmerge_context *ctx,struct track *track,int chid) {
  if (track->p>=track->c) return -1;
  
  int explicit_status=0;
  int status=track->v[track->p];
  if (status&0x80) { track->p++; explicit_status=1; }
  else if (track->status) status=track->status;
  else return -1; // Invalid leading byte.
  track->status=status;
  
  switch (status&0xf0) {
  
    case 0xa0: case 0xb0: track->p+=2; break; // Ignore Note Adjust and Control Change.
    case 0xc0: case 0xd0: track->p+=1; break; // Ignore Program Change and Channel Pressure.
    
    case 0x80: case 0x90: case 0xe0: { // Note On, Note Off, Wheel: Emit, with chid replaced.
        if (midmerge_write_delay(ctx)<0) return -1;
        if (explicit_status) {
          if (sr_encode_u8(&ctx->dst,(status&0xf0)|chid)<0) return -1;
        }
        if (track->p>track->c-2) return -1;
        if (sr_encode_raw(&ctx->dst,track->v+track->p,2)<0) return -1;
        track->p+=2;
      } break;
    
    case 0xf0: {
        track->status=0;
        int type=0;
        if (status==0xff) {
          if (track->p>=track->c) return -1;
          type=track->v[track->p++];
        }
        int len,lenlen;
        if ((lenlen=sr_vlq_decode(&len,track->v+track->p,track->c-track->p))<0) return -1;
        track->p+=lenlen;
        if (track->p>track->c-len) return -1;
        // Drop all Sysex and most Meta...
        if (chid==0) {
          if ((type==0x51)&&(len==3)) { // Retain Set Tempo on channel zero.
            if (midmerge_write_delay(ctx)<0) return -1;
            if (sr_encode_raw(&ctx->dst,"\xff\x51\x03",3)<0) return -1;
            if (sr_encode_raw(&ctx->dst,track->v+track->p,3)<0) return -1;
          }
        }
        track->p+=len;
      } break;
    
    default: return -1;
  }
  track->delay=-1;
  return 0;
}

/* Queue up delay in any track that needs it, process any ready events, and return the new delay to next event.
 * Returns zero at EOF.
 */
 
static int midmerge_digest_until_delay(struct midmerge_context *ctx) {
  int nextdelay=0x10000000; // not expressible as VLQ.
  struct infile *file=ctx->filev;
  int filei=ctx->filec;
  for (;filei-->0;file++) {
    struct track *track=file->trackv;
    int tracki=file->trackc;
    for (;tracki-->0;track++) {
      while (track->p<track->c) {
        if (track->delay<0) {
          if (track_read_delay(track)<0) return -1;
        }
        if (!track->delay) {
          if (midmerge_process_event(ctx,track,file->chid)<0) return -1;
        } else {
          if (track->delay<nextdelay) {
            nextdelay=track->delay;
          }
          break;
        }
      }
    }
  }
  if (nextdelay>=0x10000000) return 0; // Done!
  return nextdelay;
}

/* Remove so many ticks from all pending delays and advance current time.
 * Doesn't output anything.
 */
 
static int midmerge_delay(struct midmerge_context *ctx,int tickc) {
  ctx->now+=tickc;
  struct infile *file=ctx->filev;
  int filei=ctx->filec;
  for (;filei-->0;file++) {
    struct track *track=file->trackv;
    int tracki=file->trackc;
    for (;tracki-->0;track++) {
      if (track->p>=track->c) continue;
      if (track->delay<0) {
        if (track_read_delay(track)<0) return -1;
      }
      if (track->delay>=tickc) track->delay-=tickc;
      else track->delay=0;
    }
  }
  return 0;
}

/* Write terminator.
 */
 
static int midmerge_write_terminator(struct midmerge_context *ctx) {
  if (midmerge_write_delay(ctx)<0) return -1;
  if (sr_encode_raw(&ctx->dst,"\xff\x2f\0",3)<0) return -1; // End Of Track
  return 0;
}

/* Intermediate processing.
 * All tracks are initialized and ready to read.
 * (ctx->dst) is empty and we populate it.
 */
 
static int midmerge_combine_files(struct midmerge_context *ctx) {

  if (ctx->filec<1) {
    fprintf(stderr,"%s: No input files\n",__func__);
    return -1;
  }

  // Confirm all files have the same division, we're going to depend on that.
  int i=ctx->filec;
  while (i-->1) {
    if (ctx->filev[i].division!=ctx->filev[0].division) {
      fprintf(stderr,"Division mismatch! %d for %s, and %d for %s\n",ctx->filev[i].division,ctx->filev[i].path,ctx->filev[0].division,ctx->filev[0].path);
      return -1;
    }
  }
  
  // Write MThd and a provisional MTrk header. We're emitting just one track.
  if (sr_encode_raw(&ctx->dst,"MThd\0\0\0\6\0\1\0\1",12)<0) return -1;
  if (sr_encode_intbe(&ctx->dst,ctx->filev[0].division,2)<0) return -1;
  if (sr_encode_raw(&ctx->dst,"MTrk",4)<0) return -1;
  int lenp=ctx->dst.c;
  if (sr_encode_raw(&ctx->dst,"\0\0\0\0",4)<0) return -1;
  
  /* Interleave events from tracks across all files until we run out.
   * Set Tempo will be preserved for channel zero.
   * Other Meta, and other events we know Egg doesn't need, will be dropped.
   * Note On, Note Off, and Wheel will be preserved but channel modified.
   * We're dropping End Of Track -- going to append our own after.
   */
  ctx->wtime=0;
  ctx->now=0;
  for (;;) {
    int err=midmerge_digest_until_delay(ctx);
    if (err<0) {
      fprintf(stderr,"Failed to merge MIDI files.\n");
      return err;
    }
    if (!err) break; // eof
    midmerge_delay(ctx,err);
  }
  if (midmerge_write_terminator(ctx)<0) return -1;
  
  /* Insert MTrk length.
   */
  int len=ctx->dst.c-(lenp+4);
  if (len<0) return -1;
  uint8_t *dst=(uint8_t*)(ctx->dst.v)+lenp;
  dst[0]=len>>24;
  dst[1]=len>>16;
  dst[2]=len>>8;
  dst[3]=len;
  
  return 0;
}

/* Main.
 */
 
int tool_main_midmerge() {
  if (!g.srcpath||!g.dstpath) {
    fprintf(stderr,"%s: srcpath and dstpath required for midmerge\n",g.exename);
    return -1;
  }
  struct midmerge_context ctx={0};
  int err=dir_read(g.srcpath,cb_midmerge_input,&ctx);
  if (err<0) {
    midmerge_context_cleanup(&ctx);
    return -1;
  }
  if (midmerge_combine_files(&ctx)<0) {
    midmerge_context_cleanup(&ctx);
    return -1;
  }
  if (file_write(g.dstpath,ctx.dst.v,ctx.dst.c)<0) {
    fprintf(stderr,"%s: Failed to write file, %d bytes\n",g.dstpath,ctx.dst.c);
    midmerge_context_cleanup(&ctx);
    return -1;
  }
  midmerge_context_cleanup(&ctx);
  return 0;
}
