//Ahmad As Sami
//SFU ID: 301404717
//CMPT 300 Assignment 2
//Cshell Program!


#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define buf_size 1024
#define $cshell "cshell$ "

//We can store the environment values like a struct, and 
//can add an extra variable next, to store as a linked list
//to avoid memory leaks
struct env_var
{
    char *name;
    char *value;
    struct env_var *next;
};


//Same for the command, we can store the command as a struct as specified
//in the assignment. We can add a next to store as linked list
struct command
{
    char *name;
    struct tm time;
    int code;
    struct command *next;
};

struct color
{
    const char *name;
    int theme;
};

static char global_buffer[buf_size];
static int global_theme = 0;
static struct env_var *global_environmnt_ll = NULL;
static struct command *global_log_head = NULL;
static struct command *global_log_tail;
char* maincommandptr;


//We can define ANSI escape codes, where we begin with ANSI_THEME_BEGIN, where we replace
//the "%d" with the global theme, and change the stdout.

//At the ANSI_THEME_BEGIN, we will start the color scheme from the global theme,
//by printing the global theme. Then, at ANSI_THEME_END, we will again now reset the 
//global theme to 0.
#define ANSI_THEME_BEGIN(out) \
    if (global_theme != 0) \
        fprintf(out, "\x1b[%dm", global_theme)

#define ANSI_THEME_END(out) \
    if (global_theme != 0) \
        fputs("\x1b[0m", out)

void interactive_mode(void);
static char *prompt_line(void);
static int script_mode(const char *filename);
static int process_line(char *line);
static const char *get_var(const char *name);
static void assign_var(const char *name, const char *value);
static struct env_var *var_new(const char *name, const char *value);
static int builtin_log(void);
static int builtin_print(char* line);
static int builtin_theme(const char *color);
static int run_non_built_in_command(char* reqcmd);
static void free_memory(void);



//the main, where we will use the number of arguments to 
//decide to proceed to the interactive mode, or the script mode.
int main(int argc, char *argv[])
{

    assert(argc > 0);
    assert(argv != NULL);
    assert(argv[0] != NULL);

   //If the number of arguments is only 1, which means
   //only the executable, this means that our shell will execute
   //in interactive mode.
   if (argc==1)
   {
       interactive_mode();
   }
   else if(argc>1)
   {
       assert(argv[1] != NULL); 
       script_mode(argv[1]);
   }
   else 
   {
       printf("Failed to read command line.");
   }
    return 0;


}

// This is the interactive mode of the shell,
//Where commands are parsed and executed in the command line.
void interactive_mode(void)
{
    
    char *line;

    //The while loop will continue looping until "exit" is called
    //from the process_line function, which will have a value of 1.
    //At that point the loop will break.
    while ((line = prompt_line()) != NULL)
    {
        if (process_line(line))
        {
            break;
        }
    }

    free_memory();

    return;
}


//This will be our prompt line function,
//Which will prompt a command line in the terminal
//It will first put the cshell prompt, and 
//return the command/text the user enters,
//which will then enter the process line 
//in the interactive mode function.
char *prompt_line(void)
{
    ANSI_THEME_BEGIN(stdout);
    fputs($cshell, stdout);
    ANSI_THEME_END(stdout);
    return fgets(global_buffer, buf_size, stdin);
}



//This will create the script mode of the terminal,
//When a file from outside will be sent in to the program.
//We send in the filename, which is in the second argument of argv.
int script_mode(const char *filename)
{
    int retval;
    char *line;
    FILE *scriptfile;
    scriptfile = fopen(filename, "r");

    if (scriptfile != NULL)
    {
        //The while loop will continue until every line in the line is complete,
        //or if the line is invalid
        while ((line = fgets(global_buffer, buf_size, scriptfile)) != NULL)
        {   
            //Process_line will continue until exit command is executed,
            //after which the exitcode will come in which is 0 and the loop will break.
            if (process_line(global_buffer))
            {
                break;
            }
        }

        retval=EXIT_SUCCESS;
        fclose(scriptfile);
        free_memory();
    }

    else
    {
        printf("Failed to open.");

        retval=EXIT_FAILURE;
    }

    return retval;
}

//This function acts as a helper mainly in the print,
// to keep an intact copy of the line which is sent in
//from the terminal
void commandbuffercopy(char *line)
{
     
    maincommandptr=(char*)malloc((strlen(line))*sizeof(char));
    strncpy(maincommandptr,line,strlen(line));

    for (int i=0;i<strlen(maincommandptr);i++)
    {
        if(maincommandptr[i]=='\n')
        {
            maincommandptr[i]='\0';
        }
    }


}


//Process the line which comes in as a cshell command, and parse/separate 
//it into tokens. If it starts with a $, we use variable assignment,
// or we move on to use builtin or non builtin commands. If exit
//from cshell is requested, we return 1, or 0 otherwise.
int process_line(char *line)
{
    commandbuffercopy(line);
    int is_exit = 0;
    char *commandptr;
    char *varname;
    time_t now;
    struct command *cmd;

    assert(line != NULL);
   
  
    time(&now);

    //Using strchr, we get the command until the newline command, which 
    //is placed at the end
    varname = strchr(line, '\n');
    if (varname != NULL)
        *varname = '\0';
    
    commandptr=varname;
    cmd = malloc(sizeof (struct command));
    if (cmd == NULL)
        abort();



    //CASE 1: IF VARIABLE ASSIGNMENT
    if (line[0] == '$')
    {
   

        //We create a duplicate of the line using strdup, and add it into our command name 
        cmd->name = strdup(line);
        if (cmd->name == NULL)
            abort();
        ++line;
        varname = strchr(line, '=');
        if (varname != NULL)
        {
            *varname = '\0';
            //We then assign the variable, which is after the equal
            assign_var(line, varname + 1);
            //We put a correct exit code, as there is no command to run.
            cmd->code = EXIT_SUCCESS;
        }

        else
        {
            printf("Invalid variable. abort");

            cmd->code = EXIT_FAILURE;
        }
    
    }




    
    //Case 2: IF BUILT IN OR OTHER COMMAND PROVIDED
    else
    {
        
        //Using the blank token, we get the first token of the line before whitespace,
        //which is our required command.
        commandptr = strtok(line, " ");
        if (commandptr == NULL)
        {
            cmd->name = malloc(1);
            cmd->name[0] = '\0';
            cmd->code = EXIT_SUCCESS;
        }

        else if (strcmp(commandptr, "exit") == 0)
        {
            cmd->name = NULL; 
            //We turn the exit variable to 1, and this 
            //will cause the interactive/script mode condition
            //to fail and the shell will terminate
            is_exit = 1;
        }

        else if (strcmp(commandptr, "log") == 0)
        {
            cmd->name = strdup(commandptr);
            cmd->code = builtin_log();
        }

        else if (strcmp(commandptr, "print") == 0)
        {
           
            cmd->name = strdup(commandptr);
            cmd->code=builtin_print(maincommandptr);

        }

        else if (strcmp(commandptr, "theme") == 0)
        {
            cmd->name = strdup(commandptr);
            commandptr = strtok(NULL, " ");

            if (commandptr == NULL)
            {
                ANSI_THEME_BEGIN(stderr);
                fputs("Usage: theme <COLOR>\n", stderr);
                ANSI_THEME_END(stderr);

                cmd->code = EXIT_FAILURE;
            }

            else
                cmd->code = builtin_theme(commandptr);
        }

        else
        {

            cmd->name = strdup(commandptr);
            cmd->code = run_non_built_in_command(commandptr);
        }
    }



    //Here, we will update the log, and since log
    //is a linked list, we add either to the tail,
    //or if it is an empty linked list,
    //we add to the head.
    localtime_r(&now, &cmd->time);
    cmd->next = NULL;

    if (global_log_head != NULL)
    {
        assert(global_log_tail != NULL);
        global_log_tail->next = cmd;
    }

    else
    {
        global_log_head = cmd;
    }
    global_log_tail = cmd;

    return is_exit;
}

//The following functions will be utilized for working
//with the environmental variables.


//We get the value of the environment varaible, and return
//the pointer to the value string.
const char *get_var(const char *name)
{
    struct env_var *var;
    const char *value = NULL;
    //We checked the linked list for global environment variables,
    //using strcmp, comparing the name field of the var struct,
    //and if we find the variable, we return it, otherwise we 
    //say it does not exist.
    for (var = global_environmnt_ll; var != NULL; var = var->next)
    {
        if (strcmp(var->name, name) == 0)
        {
            value = var->value;
            break;
        }
    }

    if (value == NULL)
    {
        ANSI_THEME_BEGIN(stderr);
        fprintf(stderr, "Variable $%s does not exist\n", name);
        ANSI_THEME_END(stderr);
    }

    return value;
}



//We assign a value to a variable, and create and add it to the end
//of the linked list for global variables
void assign_var(const char *name, const char *value)
{
    struct env_var *var;

    assert(name != NULL);
    assert(value != NULL);

    if (global_environmnt_ll == NULL)
        global_environmnt_ll = var_new(name, value);

    else
    {
        for (var = global_environmnt_ll; ; var = var->next)
        {
            if (strcmp(var->name, name) == 0)
            {
                free(var->value);

                var->value = strdup(value);
                if (var->value == NULL)
                    abort();

                break;
            }

            else if (var->next == NULL)
            {
                var->next = var_new(name, value);
                break;
            }
        }
    }
}

//This creates a new variable record, which is created using a struct,
//and will be attached to the end of the variable linked list.
//Asserts are used to make sure data is correct
struct env_var *var_new(const char *name, const char *value)
{
    struct env_var *var;

    assert(name != NULL);
    assert(value != NULL);

    var = malloc(sizeof (struct env_var));
    if (var == NULL)
        abort();

    var->name = strdup(name);
    if (var->name == NULL)
        abort();

    var->value = strdup(value);
    if (var->value == NULL)
        abort();

    var->next = NULL;

    return var;
}



//The following function is for the built_in log, which
//iterates over the linked list for log data, and prints all the data
//according to our string arrays we have for days and months.
int builtin_log(void)
{
    char* days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    char* months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun","Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    struct command *cmd;

    ANSI_THEME_BEGIN(stdout);
    for (cmd = global_log_head; cmd != NULL; cmd = cmd->next)
    {
        printf(
            "%s %s %d %02d:%02d:%02d %d\n" " %s %d\n",days[cmd->time.tm_wday], months[cmd->time.tm_mon],
            cmd->time.tm_mday, cmd->time.tm_hour, cmd->time.tm_min,
            cmd->time.tm_sec, 1900 + cmd->time.tm_year, cmd->name,cmd->code);
    }
    ANSI_THEME_END(stdout);

    return EXIT_SUCCESS;
}




int builtin_print(char* line)
{
    size_t buffsize = 0;
    char *buffer = NULL;
    char *temp;
    int i=7;

   //We know that the input is in the form of print .....
   //So we find last occurence of the space,and add 1 to the ptr to take us into the required 
   //string to print.

   //If the line has a $, which means variable value has to be printed,
   //We keep using realloc and create a string out of the variable name.
   //We then send the the variable name to the get_var function and get
   //the variabl value and print it.
   if(line[6]=='$')
    {
        while((line[i])!='\0')
        {
            ++buffsize;
            temp = realloc(buffer, (buffsize + 1) * sizeof(char));

            buffer = temp;
            buffer[buffsize - 1] = line[i];

            i++;
            

        }

        if (buffsize) {
            buffer[buffsize] = '\0';
            } 

        

    for (int i=0;i<strlen(buffer);i++)
    {
        if(buffer[i]=='\n')
        {
            buffer[i]='\0';
        }
    }
    ANSI_THEME_BEGIN(stdout);
    printf("%s",get_var(buffer));
     ANSI_THEME_END(stdout);

    }
   

    //For normal printing, we simply parse the command and print the characters after
    //"print "
    else
    {
        ANSI_THEME_BEGIN(stdout);
  
        for (int i=6;i<strlen(line);i++)
            {
                fputc(line[i],stdout);
            }

        ANSI_THEME_END(stdout);

    }

    printf("\n");
    free(line);

    return EXIT_SUCCESS;

}



//Following function makes the built-in theme command
int builtin_theme(const char *color)
{
    
    char* colors[6]={"reset","black","red","green","yellow","blue"};
    int colorval[6]={0,30,31,32,33,34};

    int exit_code = EXIT_FAILURE;

    assert(color != NULL);

    //We supply the color needed to change the global color theme variable.
    //This will in turn supply the number needed in the ANSI escape code,
    //To change color of the stdout.
    if(color!=NULL)
    {
        for (int i=0;i<6;i++)
        {
            if(strcmp(color,colors[i])==0)
            {
                global_theme=colorval[i];
                exit_code=EXIT_SUCCESS;
                break;
            }

        }

        if (exit_code != EXIT_SUCCESS)
        {
            printf("unknown theme entered! \n");
        }
    }

    return exit_code;
}


//Creates and executes a command not built-in to the shell
int run_non_built_in_command(char* reqcmd)
{
    int fds[2], exit_code,status;
    pid_t pid;
    ssize_t n_bytes;


    if (pipe(fds) == 0)
    {
        pid = fork();

        //If PID<0, it means that fork has failed.
        if (pid < 0)
        {

            ANSI_THEME_BEGIN(stderr);
            printf("Failed to fork.");
            ANSI_THEME_END(stderr);

            close(fds[0]);
            close(fds[1]);

            exit_code = EXIT_FAILURE;
        }

        //This is the child process, where we replace process with new process using exec
        else if (pid == 0)
        {
            dup2(fds[1], STDOUT_FILENO);
            dup2(fds[1], STDERR_FILENO);

            close(fds[0]);
            close(fds[1]);
            //Replacing the process executing the new command
            char *args[]={reqcmd,NULL}; 
            execvp(args[0],args); 


            //This will not be executed if execvp is successful
            printf("Missing keyword or command, or permission problem.\n");

        
            free_memory();

            exit(EXIT_FAILURE);
        }


        //The parent process, where we at first wait for the child proces
        //to finish executing.
        else
        {
            //At first we close the write end of the pipe
            close(fds[1]);

            //We wait for the status of the process to change. We also check for
            //specific process completion status; whether the process exited normally
            //or was sigkilled.
            do
            {
                waitpid(pid, &status, 0);
            }
            while (!(WIFEXITED(status) || WIFSIGNALED(status)));

            if (WIFEXITED(status))
                exit_code = WEXITSTATUS(status);

            else
                exit_code = -WTERMSIG(status);

          
            //Once we are done with the wait, we write the output of the command process to STDOUT_FILENO
            ANSI_THEME_BEGIN(stdout);
            fflush(stdout);
            while ((n_bytes = read(fds[0], global_buffer, buf_size)) > 0)
                write(STDOUT_FILENO, global_buffer, (size_t)n_bytes);
            ANSI_THEME_END(stdout);

            close(fds[0]);
        }
    }

    //Pipe not created successfully.
    else
    {
        ANSI_THEME_BEGIN(stderr);
        printf("Failed to create pipe.");
        ANSI_THEME_END(stderr);

        exit_code = EXIT_FAILURE;
    }

    return exit_code;
}

//Frees all memory used by global variables
void free_memory(void)
{
    struct env_var *current_var, *next_var;
    struct command *current_cmd, *next_cmd;

    for (
        current_var = global_environmnt_ll;
        current_var != NULL;
        current_var = next_var
    )
    {
        next_var = current_var->next;

        free(current_var->name);
        free(current_var->value);
        free(current_var);
    }

    for (
        current_cmd = global_log_head;
        current_cmd != NULL;
        current_cmd = next_cmd
    )
    {
        next_cmd = current_cmd->next;

        free(current_cmd->name);
        free(current_cmd);
    }
}
