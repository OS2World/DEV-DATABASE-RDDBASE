/* ---------- "C source to read dBASE files" ---------- */
/* Here is the C code that allows you to read a DBASE-III file. I verified
that it works under Microsoft C v4.0. Hope it does every thing you need.
I am not sure where it came from, so hopefully I am not violating anyones
copyrights :-). Have fun.

Greg Twaites
SICOM Inc.
!ihnp4!sun!sunburn!sicom!twaites
*/

/*									     */
/*									     */
/* changed IO and MEMORY calls to OS/2 calls by Joe McVerry 919-846-2014     */
/*									     */
/* compiled with MSC 5.1 - use the /AL or some form of far pointer allocation*/
/*									     */
/*									     */


/*
 * These functions are used to read dbase files.
 *
 * These functions are provided by Valour Software as a gift.
 *
 * The program is for test purposes only.  No warranty is expressed nor
 * implied. USE AT YOUR OWN RISK!
 *
 *
 */

#define LINT_ARGS
 
#define INCL_BASE
#include <os2.h>
#include <dos.h>
#include <stdio.h>

HFILE  dbfile;
PHFILE p_file = &dbfile;

USHORT f_result;
PUSHORT pf_result = &f_result;

long rec_selector;
short subrec_selector;

long fld_selector;
short subfld_selector;

typedef  struct dbase_head {
    unsigned char   version;     /*03 for dbIII and 83 for dbIII w/memo file*/
    unsigned char   l_update[3];                    /*yymmdd for last update*/
    unsigned long   count;                       /*number of records in file*/
    unsigned int    header;                           /*length of the header
                                                       *includes the \r at end
                                                       */
    unsigned int    lrecl;                            /*length of a record
                                                       *includes the delete
                                                       *byte
                                                       */
    unsigned char   reserv[20];
    } DBASE_HEAD;
 
typedef struct dbase_fld {
    char    name[11];                                           /*field name*/
    char    type;                                               /*field type*/
#define DB_FLD_CHAR  'C'
#define DB_FLD_NUM   'N'
#define DB_FLD_LOGIC 'L'
#define DB_FLD_MEMO  'M'
#define DB_FLD_DATE  'D'
    /* A-T uses large data model but drop it for now */
    char far        *data_ptr;                         /*pointer into buffer*/
    unsigned char   length;                                   /*field length*/
    unsigned char   dec_point;                         /*field decimal point*/
    unsigned char   fill[14];
    } DBASE_FIELD;
 
typedef struct fld_list {
    struct fld_list *next;
    DBASE_FIELD     *fld;
    char            *data;
    } FLD_LIST;
 
DBASE_HEAD  dbhead={0};
FLD_LIST    *db_fld_root=0;
char        *Buffer;
char        buf_work[255];
int	    rc;
 
/*------------------------------------------------------------code-------*/
main(argc,argv)
int     argc;
char    *argv[];
{
    if(argc!=2)
       {
	printf("Usage is db_dump filename.dbf");
	exit(100);
       }
    rc = DosOpen(argv[1],
		  p_file,pf_result,(ULONG)0,0,0x01,0x00c0,(ULONG)0);
    if(rc)
       {
	printf("Dos open error %3.3d for file %s ",rc,argv[1]);
	exit(150);
       }
    db3_read_dic();
    db3_print_recs(dbhead.count);
    printf("\n\n\t\t\tProcessing complete");
    DosClose(dbfile);
    exit(0);
}
 
 
/******************************************************
                                         db3_read_dic()
This function is called with a file name to
read to create a record type to support the
dbase file
******************************************************/
 
db3_read_dic()
{
    int 	    len;
    int             fields;
    DBASE_FIELD     *fld;

    int 	    alloc_size;

    if(p_file == NULL) {
        printf("open failed");
        exit(200);
        }
    rc = DosRead(dbfile,&dbhead,sizeof(DBASE_HEAD),&len);
    if (rc)
      {
	printf("\n\aUnable to read file rc = %3.3d",rc);
	exit(205);
      }
    if( (dbhead.version!=3 && dbhead.version!=0x83) )
       {
	printf("\n\aVersion %d not supported",dbhead.version);
	exit(300);
       }

    printf("update year    %3d\n",dbhead.l_update[0]);
    printf("update mon     %3d\n",dbhead.l_update[1]);
    printf("update day     %3d\n",dbhead.l_update[2]);
    printf("number of recs %3d\n",dbhead.count);
    printf("header length  %3d\n",dbhead.header);
    printf("record length  %3d\n",dbhead.lrecl);

    fields=(dbhead.header-1)/32-1;

    /* allocate space for file buffer area */
    rc = DosAllocSeg(dbhead.lrecl,(PSEL)&rec_selector,0);
    if (rc)
       {
	printf("\n\aUnable to allocate memory rc = %3.3d",rc);
	exit(400);
       }

    FP_SEG(Buffer) = rec_selector;
    FP_OFF(Buffer) = 0;

    /* allocate space and sub area for record buffer area
       reuse selector but don't corrupt it again or you're in big trouble */

    alloc_size = (fields+1) * sizeof(FLD_LIST);

    rc = DosAllocSeg(alloc_size,(PSEL)&rec_selector,0);
    if (rc)
       {
	printf("\n\aUnable to allocate record memory rc = %3.3d",rc);
	exit(500);
       }

    DosSubSet(rec_selector,1,alloc_size);
    if (rc)
      {
	printf("\n\aUnable to set suballocated record memory rc = %3.3d",rc);
	exit(505);
      }

    // allocate space for data to stored in

    alloc_size = (fields+1) * sizeof(DBASE_FIELD);

    rc = DosAllocSeg(alloc_size,(PSEL)&fld_selector,0);
    if (rc)
      {
	printf("\n\aUnable to allocate field memory rc = %3.3d",rc);
	exit(600);
      }

    DosSubSet(fld_selector,1,alloc_size);
    if (rc)
      {
	printf("\n\aUnable to set suballocated field memory rc = %3.3d",rc);
	exit(605);
      }

    printf("\nField Name\tType\tLength\tDecimal Pos\n");
    while(fields--) {
	rc = DosSubAlloc(fld_selector,(PUSHORT)&subfld_selector,sizeof(DBASE_FIELD));
	if (rc)
	  {
	    printf("\n\aUnable to suballocate field memory rc = %3.3d",rc);
	    exit(610);
	  }

	FP_SEG(fld) = fld_selector;
	FP_OFF(fld) = subfld_selector;

	rc = DosRead(dbfile,fld,sizeof(DBASE_FIELD),&len);
	if (rc)
	  {
	    printf("\n\aUnable to read file rc = %3.3d",rc);
	    exit(620);
	  }
	printf("%-10s\t  %c\t  %3d\t  %3d\n",fld->name,fld->type,
                                     fld->length,fld->dec_point);
	stack_field(fld);
    }
    rc = DosRead(dbfile,Buffer,1,&len);     /*read the silly little \r character*/
    if (rc)
      {
       printf("\n\aUnable to read file rc = %3.3d",rc);
       exit(630);
      }
    return;
}
 
/******************************************************
                                        db3_print_recs()
Read records and print the data
******************************************************/
 
db3_print_recs(cnt)
int     cnt;
{
    int     bytes;
 
    while(cnt) {
	rc=DosRead(dbfile,Buffer,dbhead.lrecl,&bytes);
	if (rc)
	  {
	    printf("\n\aUnable to read file rc = %3.3d",rc);
	    exit(700);
	  }
        if(bytes!=dbhead.lrecl)
            break;
        if(Buffer[0]!='*') {
            db3_print();
            cnt--;
            }
        }
    return;
}
 
 
/******************************************************
                                          db3_print()
Print a single record
******************************************************/
 
db3_print()
{
    FLD_LIST    *list, *temp;
 
    temp=db_fld_root;
    printf("\n");
    while(temp) {
        memcpy(buf_work,temp->data,temp->fld->length);
        buf_work[temp->fld->length]='\0';
        printf("%-10s=%s\n",temp->fld->name,buf_work);
        temp=temp->next;
        }
    return;
}
 
/******************************************************
                                         stack_field()
Add a field to the linked list of fields
******************************************************/
 
stack_field(fld)
DBASE_FIELD *fld;
{
    FLD_LIST    *list, *temp;
    long fld_selector;
    rc = DosSubAlloc(rec_selector,(PUSHORT)&subrec_selector,sizeof(FLD_LIST));
    if (rc)
       {
	printf("\n\aUnable to suballocate record memory rc = %3.3d",rc);
	exit(800);
       }
    FP_SEG(list) = rec_selector;
    FP_OFF(list) = subrec_selector;
    list->next = NULL;
    list->fld=fld;
    if(!db_fld_root) {
        list->data=Buffer+1;                            /*skip delete byte*/
        db_fld_root=list;
        return;
        }
    temp=db_fld_root;
    while(temp->next)
        temp=temp->next;
    temp->next=list;
    list->data=temp->data + temp->fld->length;
    return;
}
 
 
/* End of text from inmet:comp.sys.ibm.pc */
