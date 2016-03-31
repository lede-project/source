
/* Documentation */
const char *argp_program_version =  "caldata utils 1.0";
static char doc[] =  "caldata - Utility to manipulate calibration data for ath10k";
static char args_doc[] = "ARG1 ARG2";

/* Options */
static struct argp_option options[] = {
  {"input",  	'i', "PARTITION/FILE",	0,  "Read from PARTITION or FILE" },
  {"output",	'f', "FILE",	0,  "Output to FILE." },
  {"offset",	'o', "BYTES",	0,  "Skip first BYTES" },
  {"size",		's', "BYTES",	0,	"Size of data to read"},
  {"maddress",	'a', "ADDRESS",	0,	"new MAC address to assign"},
  {"verify",	'v', 0,			0,	"Only verify input, no output"},
  { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
  char *input_file;
  char *output_file;

  long int offset;
  long int size;
  char *macaddr;
  int newmac[6];
  
  int  verify;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state);

static struct argp argp = { options, parse_opt, args_doc, doc };


// Utils

int isMAC(char *s);
int getMTD(char *name);
unsigned short checksum(void *caldata, int size);

