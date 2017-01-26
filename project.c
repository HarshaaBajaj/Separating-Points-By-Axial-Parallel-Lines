#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#define	MAX_PTS	100
#define	unable_to_read_all_points_err "Unable to read all points in the file\n"
#define	input_file_not_found_err "Instance file [%d] not found\n"
#define	no_points_to_read_err "There are no points in file %d\n"

typedef struct POINT_INFO POINT_INSTANCE;
typedef struct LINE_INFO LINE_INSTANCE;

struct LINE_INFO
{
	float	LINE_AXIS_INTER;
	int		LINE_IS_ON_AXIS;
	int		IS_LINE_ADDED_TO_SOLUTION;
};

struct POINT_INFO {
	int		X_COOR;
	int		Y_COOR;
	POINT_INSTANCE	**CON_POINTS;
	LINE_INSTANCE	*LEFT_ADJ_LINE;
	LINE_INSTANCE	*RIGHT_ADJ_LINE;
	LINE_INSTANCE	*BOTTOM_ADJ_LINE;
	LINE_INSTANCE	*TOP_ADJ_LINE;
	int		CNT_OF_CONS;
	int		SELF_CON;
	
};

static LINE_INSTANCE LINES_OVERX[MAX_PTS];
static LINE_INSTANCE LINES_OVERY[MAX_PTS];
static LINE_INSTANCE *FEASIBLE_SOL[MAX_PTS];

static POINT_INSTANCE ARRAY_OF_ALL_POINTS[MAX_PTS];
static POINT_INSTANCE *SORTED_BY_XCOOR[MAX_PTS];
static POINT_INSTANCE *SORTED_BY_YCOOR[MAX_PTS];

typedef enum AXIS_LINE {
	X_AXIS,
	Y_AXIS
} axis_temp;

typedef enum RETURN_VALUES {
	success,
	no_points_to_read,
	unable_to_read_all_points,
	input_file_not_found,
} return_valurs_temp;

static int CNT_POINTS = 0;

static int XCOOR_TAKEN[MAX_PTS];
static int YCOOR_TAKEN[MAX_PTS];

static int CNT_LINES_X = 0;
static int CNT_LINES_Y = 0;
static int CNT_LINES_TOTAL = 0;

static int REMAINING_CONS = 0;

static void MAKE_CONS()
{
	int i = 0;
	int j = 0;
	
	while (i < CNT_POINTS) 
	{
		ARRAY_OF_ALL_POINTS[i].CON_POINTS = malloc(sizeof (POINT_INSTANCE *)*MAX_PTS);
		i++;
	}
	i = 0;
	while (i < CNT_POINTS) {
		j = 0;
		ARRAY_OF_ALL_POINTS[i].SELF_CON = i;
		while (j < CNT_POINTS) {
			if (i != j) {
				ARRAY_OF_ALL_POINTS[i].CON_POINTS[j] = &(ARRAY_OF_ALL_POINTS[j]);
				ARRAY_OF_ALL_POINTS[i].CNT_OF_CONS++;
				REMAINING_CONS++;
			} 
			else {
				ARRAY_OF_ALL_POINTS[i].CON_POINTS[j] = NULL;
			}
			j++;
		}
		i++;
	}
	if (REMAINING_CONS != (CNT_POINTS*(CNT_POINTS-1))) {
		(void) printf("REMAINING_CONS=%d, but should be %d\n", REMAINING_CONS,
			    (CNT_POINTS*(CNT_POINTS-1)));
		exit(0);
	}

}

static int NEAREST_LEFT(int AXIS_LINE, float INTERSECTION)
{
	int i = 0;
	float coordinate = 0;
	POINT_INSTANCE **temp;
	if (AXIS_LINE == X_AXIS) {
		temp = SORTED_BY_XCOOR;
	} 
	else {
		temp = SORTED_BY_YCOOR;
	}
	while (i < CNT_POINTS) {
		if (AXIS_LINE == X_AXIS) {
			coordinate = (float)temp[i]->X_COOR;
		} 
		else {
			coordinate = (float)temp[i]->Y_COOR;
		}
		if ((float)coordinate > INTERSECTION) {
			return ((i-1));
		}
		i++;
	}
	return (-1);
}

static int COMPARE_XCOORS(const void *pt1, const void *pt2)
{
	POINT_INSTANCE **point1 = (POINT_INSTANCE **)pt1;
	POINT_INSTANCE **point2 = (POINT_INSTANCE **)pt2;

	if ((*point1)->X_COOR > (*point2)->X_COOR) {
		return (1);
	}

	if ((*point1)->X_COOR < (*point2)->X_COOR) {
		return (-1);
	}

	return (0);

}

static int COMPARE_YCOORS(const void *pt1, const void *pt2)
{
	POINT_INSTANCE **point1 = (POINT_INSTANCE **)pt1;
	POINT_INSTANCE **point2 = (POINT_INSTANCE **)pt2;

	if ((*point1)->Y_COOR > (*point2)->Y_COOR) {
		return (1);
	}

	if ((*point1)->Y_COOR < (*point2)->Y_COOR) {
		return (-1);
	}

	return (0);
}

static void SORT()
{

	qsort(SORTED_BY_XCOOR, CNT_POINTS, sizeof (POINT_INSTANCE *),
		&COMPARE_XCOORS);

	qsort(SORTED_BY_YCOOR, CNT_POINTS, sizeof (POINT_INSTANCE *),
		&COMPARE_YCOORS);

}

static void RESET()
{
	int i = 0;
	while (i < CNT_POINTS) {
		free(ARRAY_OF_ALL_POINTS[i].CON_POINTS);
		i++;
	}
	CNT_POINTS = 0;
	CNT_LINES_TOTAL = 0;
	CNT_LINES_X = 0;
	CNT_LINES_Y = 0;
	REMAINING_CONS=0;
}

static int READ_INSTANCE(int n)
{
	int x = 0;
	int y = 0;
	int i = 0;
	char file_path[255];
	char *ch = &file_path[0];
	(void) sprintf(ch, "./input/instance%.2d", n);
	FILE *fptr = fopen(ch, "r");
	if (fptr == NULL) {
		return (input_file_not_found);
	}

	int ret = fscanf(fptr, "%d", &CNT_POINTS);
	
	if (ret == EOF) {
		return (no_points_to_read);
	}

	while (i < MAX_PTS) {
		XCOOR_TAKEN[i] = 0;
		YCOOR_TAKEN[i] = 0;
		i++;
	}
	i = 0;
	while (fscanf(fptr, "%d %d", &x, &y) != EOF && i < MAX_PTS) {
		ARRAY_OF_ALL_POINTS[i].X_COOR = x;
		XCOOR_TAKEN[x] = 1;
		ARRAY_OF_ALL_POINTS[i].Y_COOR = y;
		YCOOR_TAKEN[i] = 1;
		ARRAY_OF_ALL_POINTS[i].CNT_OF_CONS = 0;
		ARRAY_OF_ALL_POINTS[i].LEFT_ADJ_LINE = NULL;
		ARRAY_OF_ALL_POINTS[i].BOTTOM_ADJ_LINE = NULL;
		ARRAY_OF_ALL_POINTS[i].RIGHT_ADJ_LINE = NULL;
		ARRAY_OF_ALL_POINTS[i].TOP_ADJ_LINE = NULL;
		i++;
	}
	if (i != CNT_POINTS) {
		return (unable_to_read_all_points);
	}
	int j = 0;
	while (j < CNT_POINTS) {
		SORTED_BY_XCOOR[j] = &(ARRAY_OF_ALL_POINTS[j]);
		SORTED_BY_YCOOR[j] = &(ARRAY_OF_ALL_POINTS[j]);
		j++;
	}
	return (success);
}

static void feasible_solution(int n)
{
	int i = 0;
	char file_path[255];
	char *ch; 
	ch = &file_path[0];
	(void) sprintf(ch, "./output_greedy/greedy_solution%.2d", n);
	FILE *fptr_output = fopen(ch, "w");
	(void) fprintf(fptr_output, "%d\n", (CNT_LINES_TOTAL));
	while (i < CNT_LINES_TOTAL) {
		switch (FEASIBLE_SOL[i]->LINE_IS_ON_AXIS) {

		case X_AXIS:
			(void) fprintf(fptr_output, "v ");
			break;

		case Y_AXIS:
			(void) fprintf(fptr_output, "h ");
			break;
		}
		(void) fprintf(fptr_output, "%f\n", FEASIBLE_SOL[i]->LINE_AXIS_INTER);
		 i++;
	}
	(void) fclose(fptr_output);
}

static void discon_pts(int pt1, int pt2)
{
		if ((ARRAY_OF_ALL_POINTS[pt1]).CON_POINTS[pt2] != NULL) {
			(ARRAY_OF_ALL_POINTS[pt1]).CON_POINTS[pt2] = NULL;
			(ARRAY_OF_ALL_POINTS[pt2]).CON_POINTS[pt1] = NULL;
			(ARRAY_OF_ALL_POINTS[pt1]).CNT_OF_CONS--;
			(ARRAY_OF_ALL_POINTS[pt2]).CNT_OF_CONS--;
			REMAINING_CONS -= 2;
		}
}

static void add_to_sol(LINE_INSTANCE *ln)
{
	int i = 0;
	ln->IS_LINE_ADDED_TO_SOLUTION = 1;
	POINT_INSTANCE **temp;
	int AXIS_LINE = ln->LINE_IS_ON_AXIS;
	float INTERSECTION = (float)ln->LINE_AXIS_INTER;
	FEASIBLE_SOL[CNT_LINES_TOTAL] = ln;
	int p = NEAREST_LEFT(AXIS_LINE, INTERSECTION);
	if (AXIS_LINE == X_AXIS) {
		temp = SORTED_BY_XCOOR;
	} 
	else {
		temp = SORTED_BY_YCOOR;
		}
	int j = p;
	while (i <= p) {
		j = p+1;
		while (j < CNT_POINTS) {
			discon_pts(temp[i]->SELF_CON,temp[j]->SELF_CON);
			j++;
		}
		i++;
	}
	CNT_LINES_TOTAL++;
}

static void erase(LINE_INSTANCE *ln){
					ln->IS_LINE_ADDED_TO_SOLUTION = 0;
	}
	
static int con_check(LINE_INSTANCE *ln)
{
	int i = 0;
	POINT_INSTANCE **temp;
	int AXIS_LINE = ln->LINE_IS_ON_AXIS;
	float INTERSECTION = ln->LINE_AXIS_INTER;
	int p = NEAREST_LEFT(AXIS_LINE, INTERSECTION);
	if (AXIS_LINE == X_AXIS) {
		temp = &(SORTED_BY_XCOOR[0]);
	} else {
		temp = &(SORTED_BY_YCOOR[0]);
	}
	int j = p+1;
	while (i < (p+1)) {
		j = p+1;
		while (j < CNT_POINTS) {
			if ((ARRAY_OF_ALL_POINTS[(temp[i]->SELF_CON)].CON_POINTS[(temp[j]->SELF_CON)]) != NULL) {
				return (1);				
			}
			j++;
		}
	i++;
	}
	return (0);
}

static void partition_line(int AXIS_LINE, int from, int to)
{
	if (from == to) {
		return;
	}

	int range;
	int half;
	float xpt1;
	float xpt2;
	float ypt1;
	float ypt2;
	float diff_btw_coor;
	float pt_dist;
	float pt_coord;
	POINT_INSTANCE *pt1;
	POINT_INSTANCE *pt2;
	LINE_INSTANCE *current_line;
	POINT_INSTANCE **temp;
	if (AXIS_LINE == X_AXIS) {
		temp = &(SORTED_BY_XCOOR[0]);
		range = to - from;
		if (range == 0 || range == 1) {
			return;
		}
		if (range%2) {
			half = ((range-1)/2);
		}
		else {
			half = range/2;
		}
		if (half) {
			pt1 = (temp[(from + (half))]);
			pt2 = (temp[(from + (half+1))]);
			xpt1 = (float)pt1->X_COOR;
			xpt2 = (float)pt2->X_COOR;
			diff_btw_coor = xpt2 - xpt1;
			pt_dist = (diff_btw_coor-2)/2;
			pt_coord = xpt1 + pt_dist;
			current_line = &(LINES_OVERX[CNT_LINES_X]);
			current_line->LINE_IS_ON_AXIS = X_AXIS;
			current_line->LINE_AXIS_INTER = pt_coord;
			current_line->IS_LINE_ADDED_TO_SOLUTION = 0;
			CNT_LINES_X++;
			if (range != 2) {
				partition_line(AXIS_LINE, from, (from+half));
				partition_line(AXIS_LINE, (from+half), to);
			}
		}	
		return;
	} 
	else {
		temp = &(SORTED_BY_YCOOR[0]);
		range = to - from;
		if (range == 0 || range == 1) {
			return;
		}
		if (range%2) {
			half=((range-(range%2))/2);
		} else {
			half = range/2;
		}
		if (half) {
			pt1 = temp[(from + (half))];
			pt2 = temp[(from + (half+1))];
			ypt1 = (float)pt1->Y_COOR;
			ypt2 = (float)pt2->Y_COOR;
			diff_btw_coor = ypt2 - ypt1;
			pt_dist = (diff_btw_coor-2)/2;
			pt_coord = ypt1 + pt_dist;
			current_line = &(LINES_OVERY[CNT_LINES_Y]);
			current_line->LINE_IS_ON_AXIS = Y_AXIS;
			current_line->LINE_AXIS_INTER = pt_coord;
			current_line->IS_LINE_ADDED_TO_SOLUTION = 0;
			CNT_LINES_Y++;
			if (range != 2) {
				partition_line(AXIS_LINE, from, (from+half));
				partition_line(AXIS_LINE, (from+half), to);
			}
		}		
	return;
	}
}

int main()
{
	int INSTANCE_NUM = 1;
	while (INSTANCE_NUM < 100) {	
		int FILE_STATUS = READ_INSTANCE(INSTANCE_NUM);
		switch (FILE_STATUS) {
		case input_file_not_found:
			(void) fprintf(stderr, input_file_not_found_err,INSTANCE_NUM);
			(void) fprintf(stderr, "Quitting\n");
			exit(0);
			break;

		case unable_to_read_all_points:
			(void) fprintf(stderr, unable_to_read_all_points_err,INSTANCE_NUM);
			(void) fprintf(stderr, "Quitting\n");
			exit(0);
			break;

		case no_points_to_read:
			(void) fprintf(stderr, no_points_to_read_err,INSTANCE_NUM);
			(void) fprintf(stderr, "Quitting\n");
			exit(0);
			break;
		}

		SORT();
		MAKE_CONS();
		partition_line(X_AXIS, 0, (CNT_POINTS-1));
		partition_line(Y_AXIS, 0, (CNT_POINTS-1));
		int clx = 0;
		int cly = 0;
		int con = 0;
		while (REMAINING_CONS && clx < CNT_LINES_X && cly < CNT_LINES_Y) {
			con = con_check(&(LINES_OVERX[clx]));
			if (con) {
				add_to_sol(&(LINES_OVERX[clx]));
			}
			else{
				erase(&(LINES_OVERX[clx]));
			}
			clx++;
			con = con_check(&(LINES_OVERY[cly]));
			if (con) {
				add_to_sol(&(LINES_OVERY[cly]));
			}
			else{
				erase(&(LINES_OVERY[cly]));
			}
			cly++;
		}
		feasible_solution(INSTANCE_NUM);
		(void) printf("Solved instance %.2d\n", INSTANCE_NUM);
		RESET();
		INSTANCE_NUM++;
	}
}