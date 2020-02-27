#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include "md.h"
#include <unistd.h>
#include <sys/stat.h>

char *readfile(int fd) {
  char *p; 
  struct stat s;
  off_t len;
  ssize_t bytes_read, status;

  assert(fd>1 && "Invalid file descriptor");
  assert(0==fstat(fd, &s) && "Couldn't determine file size");
  len = s.st_size;
  assert(len>0 && "File is empty");
  p = (char *)malloc(len+1);
  bytes_read = 0;
  while( bytes_read<len ) {
    status = read(fd, &p[bytes_read], len-bytes_read);
    assert(status>=0 && "read() failed");
    bytes_read+=status;
  }
  p[len] = (char)0; // Add NULL terminator
  close(fd);
  return p;
}

char *find_section_start(char *s, int n) {
  int i=0;

  assert(n>=0 && "Invalid section number");
  if(n==0)
    return s;

  // Find the nth "%%\n" substring (if *s==0, there wasn't one)
  while(i<n && (*s)!=(char)0) {
    // This comparison will short-circuit before overrunning the string, so no length check.
    if( s[0]=='%' && s[1]=='%' && s[2]=='\n' ) {
      i++;
    }
    s++;
  }
  if(*s!=(char)0)
    return s+2; // Skip the section header itself, return pointer to the content
  return s; // Hit the end, return an empty string
}

int parse_int_array(char *s, int32_t *arr, int n) { 
  char *line, *endptr; 
  int i=0; 
  int32_t v; 
  
  assert(s!=NULL && "Invalid input string"); 
  
  line = strtok(s,"\n"); 
  while( line!=NULL && i<n ) { 
    endptr = line; 
    v = (int32_t)(strtol(line, &endptr, 10)); 
    if( (*endptr)!=(char)0 ) { 
      fprintf(stderr, "Invalid input: line %d of section\n", i); 
    } 
    arr[i] = v; 
    i++; 
    line[strlen(line)] = '\n'; /* Undo the strtok replacement.*/ 
    line = strtok(NULL,"\n"); 
  } 
  if(line!=NULL) { /* stopped because we read all the things */ 
    line[strlen(line)] = '\n'; /* Undo the strtok replacement.*/ 
  } 
  
  return 0; 
}

int parse_double_array(char *s, TYPE *arr, int n) { 
  char *line, *endptr; 
  int i=0; 
  TYPE v; 
  
  assert(s!=NULL && "Invalid input string"); 
  
  line = strtok(s,"\n"); 
  while( line!=NULL && i<n ) { 
    endptr = line; 
    v = (TYPE)(strtod(line, &endptr)); 
    if( (*endptr)!=(char)0 ) { 
      fprintf(stderr, "Invalid input: line %d of section\n", i); 
    } 
    arr[i] = v; 
    i++; 
    line[strlen(line)] = '\n'; /* Undo the strtok replacement.*/ 
    line = strtok(NULL,"\n"); 
  } 
  if(line!=NULL) { /* stopped because we read all the things */ 
    line[strlen(line)] = '\n'; /* Undo the strtok replacement.*/ 
  } 
  
  return 0; 
}

void run_benchmark() {
    struct bench_args_t args;
    char const *in_file;
    in_file = "input.data";

    int in_fd;
    in_fd = open( in_file, O_RDONLY );
    assert( in_fd>0 && "Couldn't open input data file");

    char *p, *s;
    p = readfile(in_fd);
    
    s = find_section_start(p,1);
    parse_int_array(s, (int32_t *)args.n_points, blockSide * blockSide * blockSide);
    
    s = find_section_start(p,2);
    dvector_t *position;
    position = (dvector_t *)malloc(nBlocks * densityFactor * sizeof(dvector_t));
    parse_double_array(s, (double *)position, 3 * blockSide * blockSide * blockSide * densityFactor);
    for (int i=0; i< blockSide; i++){
    for (int j=0; j< blockSide; j++){
    for (int k=0; k< blockSide; k++){
        for (int l=0; l< densityFactor; l++){
            args.position_x[i][j][k][l] = position[(i * blockSide * blockSide + j * blockSide + k) * densityFactor + l].x;
            args.position_y[i][j][k][l] = position[(i * blockSide * blockSide + j * blockSide + k) * densityFactor + l].y;
            args.position_z[i][j][k][l] = position[(i * blockSide * blockSide + j * blockSide + k) * densityFactor + l].z;
        }
    }
    }
    }
    free(position);
    free(p);
    
    md( args.n_points, args.force_x, args.force_y, args.force_z, args.position_x, args.position_y, args.position_z );
    printf("One example output is %f, %f, %f \n", args.force_x[blockSide-1][blockSide-1][blockSide-1][densityFactor-1]
                                        , args.force_y[blockSide-1][blockSide-1][blockSide-1][densityFactor-1]
                                        , args.force_z[blockSide-1][blockSide-1][blockSide-1][densityFactor-1]);
}

int main () {
    run_benchmark();
    return 1;
}
