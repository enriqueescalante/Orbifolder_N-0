//J28 v2. 
//May20 Fusion U1s into create orbi/practprompt
#include "cprompt.h"
#include "canalysemodel.h"
#include "globalfunctions.h"
#include "crandommodel.h"
#include "cinequivalentspectra.h"

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>

extern unsigned SELFDUALLATTICE;

using std::vector;
using std::cout;
using std::endl;


/* ########################################################################################
######   CPrompt(COrbifold &Orbifold)                                                ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   -                                                                           ######
######   output:                                                                     ######
######   -                                                                           ######
###########################################################################################
######   description:                                                                ######
######   Standard constructor of a CPrompt object. No content is specified.          ######
######################################################################################## */
CPrompt::CPrompt()
  : Print(Tstandard, &cout)
{
  // start the prompt in its main directory
  this->current_folder.assign(10,0);
  this->current_folder[0]  = -1;
  this->OrbifoldIndex = -1;

  this->output_filename     = "";
  this->keep_output_to_file = false;
  this->pre_exit            = false;
  this->exit                = false;
  this->online_mode         = false;
  this->print_output        = true;
  this->wait_for_processes  = false;
}



/* ########################################################################################
######   CPrompt(const COrbifold &Orbifold)                                          ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Orbifold : COrbifold object to be loaded into the prompt                 ######
######   output:                                                                     ######
######   -                                                                           ######
###########################################################################################
######   description:                                                                ######
######   Constructor of a CPrompt object. Loads "Orbifold" as a directory into the   ######
######   prompt.                                                                     ######
######################################################################################## */
CPrompt::CPrompt(const COrbifold &Orbifold)
  : Print(Tstandard, &cout)
{
  // begin: add orbifold
  this->Orbifolds.push_back(Orbifold);
  this->OrbifoldIndex = 0;

  SConfig TestConfig = Orbifold.StandardConfig;
  TestConfig.ConfigLabel = "TestConfig";
  TestConfig.ConfigNumber = 1;

  vector<SConfig> Configs;
  Configs.push_back(Orbifold.StandardConfig);
  Configs.push_back(TestConfig);
  this->AllVEVConfigs.push_back(Configs);
  this->AllVEVConfigsIndex.push_back(1);
  // end: add orbifold

  // start the prompt in the directory of "Orbifold"
  this->current_folder.assign(10,0);
  this->current_folder[0]   = 0;

  this->output_filename     = "";
  this->keep_output_to_file = false;
  this->pre_exit            = false;
  this->exit                = false;
  this->online_mode         = false;
  this->print_output        = true;
  this->wait_for_processes  = false;
}



/* ########################################################################################
######   CPrompt(const string &Filename                                              ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Filename : file name of a model file                                     ######
######   output:                                                                     ######
######   -                                                                           ######
###########################################################################################
######   description:                                                                ######
######   Constructor of a CPrompt object. Load orbifolds from model file "Filename". ######
######################################################################################## */
CPrompt::CPrompt(const string &Filename)
  : Print(Tstandard, &cout)
{
  // start the prompt in its main directory
  this->current_folder.assign(10,0);
  this->current_folder[0]  = -1;
  this->OrbifoldIndex = -1;

  this->output_filename     = "";
  this->keep_output_to_file = false;
  this->pre_exit            = false;
  this->exit                = false;
  this->online_mode         = false;
  this->print_output        = true;
  this->wait_for_processes  = false;

  (*this->Print.out) << "\n";
  this->LoadOrbifolds(Filename, false, 0);
}



/* ########################################################################################
######   ~CPrompt()                                                                  ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   -                                                                           ######
######   output:                                                                     ######
######   -                                                                           ######
###########################################################################################
######   description:                                                                ######
######   Standard destructor of a CPrompt object.                                    ######
######################################################################################## */
CPrompt::~CPrompt()
{
}



/* ########################################################################################
######   StartPrompt(string ifilename, bool stop_when_file_done, bool online_mode)   ######
######                                                                               ######
######   Version: 26.09.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) ifilename           : load commands from this file if specified          ######
######   2) stop_when_file_done : only execute commands from file then stop prompt   ######
######   3) online_mode         : for the web interface                              ######
######   output:                                                                     ######
######   return value           : prompt stoppt without problems?                    ######
###########################################################################################
######   description:                                                                ######
######   Starts the linux-style command line called the prompt.                      ######
######################################################################################## */
bool CPrompt::StartPrompt(string ifilename, bool stop_when_file_done, bool online_mode)
{
  this->online_mode        = online_mode;
  this->wait_for_processes = false;
  this->exit               = false;
  this->pre_exit           = false;

  this->Print.out  = &cout;
  this->Print.SetOutputType(Tstandard);

  // if "online_mode" than write all output to file "result.txt"
  // otherwise "keep_output_to_file" might be turned on using "@begin print to file"
  this->keep_output_to_file = this->online_mode;

  unsigned exec_command = 0;
  vector<string> Commands;

  string tmp_string1 = "";
  string command     = "";

  if (this->online_mode)
  {
    ifilename = "program.txt";
    this->output_filename  = "result.txt";
  }
  else
  {
    if (ifilename != "")
    {
      this->LoadProgram(ifilename, Commands);
      if ((Commands.size() == 0) && stop_when_file_done)
        return true;

      command   = "";
      ifilename = "";
    }
  }

  string parameter_string1 = "";
  string parameter_string2 = "";

  std::ofstream output_file;

  size_t old_number_of_commands = 0;
  bool output_file_open = false;
  bool use_cin          = false;

  while (true)
  {
    // STEP 0 //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin: online mode
    if (this->online_mode)
    {
      struct stat stFileInfo;

      old_number_of_commands = Commands.size();
      while ((exec_command == Commands.size()) || (stat(ifilename.c_str(),&stFileInfo) == 0))
      {
        // begin: wait for the file "ifilename" to exist
        if (stat(ifilename.c_str(), &stFileInfo) != 0)
        {
          cout << "  Waiting for file \"" << ifilename << "\"." << endl;

          while (stat(ifilename.c_str(), &stFileInfo) != 0)
            usleep(1000);
        }
        // end: wait for the file "ifilename" to exist

        // begin: load the commands
        usleep(5000);
        std::ifstream input(ifilename.data());
        while (GetSaveLine(input, command))
          Commands.push_back(command);
        input.close();
        // end: load the commands

        // begin: delete the file "ifilename"
        usleep(500);
        tmp_string1 = "rm ";
        tmp_string1 += ifilename;
        if (system(tmp_string1.data()) != 0)
          cout << "  System(" << tmp_string1 << ") failed." << endl;

        // end: delete the file "ifilename"
      }
      if (old_number_of_commands != Commands.size())
      {
        if (output_file_open)
        {
          (*this->Print.out) << endl << "eof" << endl;
          usleep(1000);
          output_file.close();
          usleep(7500);
        }
        output_file.open(this->output_filename.data());
        output_file_open = true;

        this->Print.out = &output_file;
      }
    }
    // end: online mode
    // STEP 0 //////////////////////////////////////////////////////////////////////////////////////////////////////////

    /*cout << endl << endl << "current commmand: " << Commands[exec_command] << endl;
    cout << "begin: all commands before ApplyConditions" << endl;
    for (unsigned q= 0; q < Commands.size(); ++q)
    cout << Commands[q] << endl;
    cout << "end: all commands before ApplyConditions" << endl;
    cout << "exec_command = " << exec_command << endl;*/

    this->ApplyConditions(Commands, exec_command);

    /*cout << "begin: all commands after ApplyConditions" << endl;
    for (unsigned q= 0; q < Commands.size(); ++q)
    cout << Commands[q] << endl;
    cout << "end: all commands after ApplyConditions" << endl;
    cout << "exec_command = " << exec_command << endl;*/

    // STEP 1 //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin: read commands
    use_cin = false;
    if (!this->online_mode && (exec_command == Commands.size()))
    {
      if (stop_when_file_done)
      {
        if (output_file_open)
        {
          (*this->Print.out) << flush;

          usleep(1000);
          output_file.close();
          output_file_open = false;
        }
        return true;
      }

      if (!this->keep_output_to_file)
      {
        this->PrintCurrentDirectory(tmp_string1);
        (*this->Print.out) << tmp_string1 << flush;
      }

      use_cin = true;
      GetSaveLine(cin, command);
      cin.clear();
      Commands.push_back(command);
    }

    command = Commands[exec_command];
    ++exec_command;
    // end: read commands
    // STEP 1 //////////////////////////////////////////////////////////////////////////////////////////////////////////

    // STEP 2 //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin: replace variables
    if (command.find("$OrbifoldLabel$", 0) != string::npos)
    {
      if ((this->Orbifolds.size() == 0) || (this->OrbifoldIndex == -1))
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Cannot replace variable \"$OrbifoldLabel$\"." << this->Print.cend << "\n" << endl;
      else
        global_ReplaceString(command, "$OrbifoldLabel$", this->Orbifolds[this->OrbifoldIndex].OrbifoldGroup.Label);
    }
    if (command.find("$VEVConfigLabel$", 0) != string::npos)
    {
      if ((this->Orbifolds.size() == 0) || (this->OrbifoldIndex == -1))
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Cannot replace variable \"$VEVConfigLabel$\"." << this->Print.cend << "\n" << endl;
      else
      {
        const SConfig &VEVConfig = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]];
        global_ReplaceString(command, "$VEVConfigLabel$", VEVConfig.ConfigLabel);
      }
    }
    if (command.find("$Directory$", 0) != string::npos)
    {
      string tmp = "";
      this->PrintCurrentDirectory(tmp);
      global_ReplaceString(command, "$Directory$", tmp);
    }
    // end: replace variables
    // STEP 2 //////////////////////////////////////////////////////////////////////////////////////////////////////////

    this->print_output = !this->FindParameterType1(command, "no output");

    // STEP 3 //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin: write output to file?
    if (this->online_mode)
    {
      if (this->FindParameterType1(command, "@begin print"))
      {
        if (this->print_output)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Command disabled in the web interface." << this->Print.cend << "\n" << endl;
        command = "";
      }
    }
    else
    {
      if (this->FindParameterType2(command, "@begin print to file(", this->output_filename))
      {
        this->keep_output_to_file = true;

        output_file.open(this->output_filename.data(), ofstream::app | ios::ate);
        output_file_open = true;

        this->Print.out = &output_file;
      }
      else
        if (this->FindParameterType2(command, " to file(", tmp_string1))
      {
        if (this->keep_output_to_file)
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Output permanently written to file \"" << this->output_filename << "\". Ignoring parameter \"to file(" << tmp_string1 << ")\"." << this->Print.cend << "\n" << endl;
        }
        else
        {
          this->output_filename = tmp_string1;
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Result written to file \"" << this->output_filename << "\"." << this->Print.cend << "\n" << endl;

          output_file.open(this->output_filename.data(), ofstream::app | ios::ate);
          output_file_open = true;

          this->Print.out = &output_file;
        }
      }
    }

    if (output_file_open && (!output_file.is_open() || !output_file.good()))
    {
      if (this->print_output)
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Cannot write to file \"" << this->output_filename << "\"." << this->Print.cend << "\n" << endl;
      this->Print.out = &cout;

      output_file.close();
      output_file_open = false;
    }
    // end: write output to file?
    // STEP 3 //////////////////////////////////////////////////////////////////////////////////////////////////////////


    // STEP 4 //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin: execute commands
    if (!this->online_mode && this->FindParameterType1(command, "@end print to file"))
    {
      (*this->Print.out) << flush;
      this->keep_output_to_file = false;

      usleep(1000);
      output_file.close();
      output_file_open = false;

      this->Print.out = &cout;
      usleep(500);
    }

    if (this->online_mode || (!use_cin && !output_file_open))
    {
      this->PrintCurrentDirectory(tmp_string1);
      (*this->Print.out) << tmp_string1 << command << endl;
    }

    if (this->FindParameterType2(command, "load program(", ifilename))
      this->LoadProgram(ifilename, Commands);

    if (this->pre_exit)
    {
      if (FindCommandType0(command, "yes") || FindCommandType0(command, "Yes"))
        this->exit = true;
      else
      {
        if (FindCommandType0(command, "no") || FindCommandType0(command, "No"))
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Program not stopped." << this->Print.cend << "\n" << endl;

          this->pre_exit = false;
        }
        else
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Do you really want to quit? Type \"yes\" to quit or \"no\" to continue." << this->Print.cend << "\n" << endl;
        }
      }
    }
    else
      this->ExecuteCommand(command);

    // end: execute commands
    // STEP 4 //////////////////////////////////////////////////////////////////////////////////////////////////////////

    usleep(100);

    if (this->online_mode && (exec_command == Commands.size()))
    {
      (*this->Print.out) << endl << "eof" << endl;

      usleep(1000);
      output_file.close();
      output_file_open = false;
      usleep(7500);
    }

    if (output_file_open && !this->keep_output_to_file)
    {
      (*this->Print.out) << flush;

      usleep(1000);
      output_file.close();
      output_file_open = false;

      this->Print.out = &cout;
      usleep(500);
    }

    if (this->exit)
    {
      if (output_file_open)
      {
        (*this->Print.out) << flush;

        usleep(1000);
        output_file.close();
        output_file_open = false;
      }

      cout << "\n  " << this->Print.cbegin << "End." << this->Print.cend << "\n" << endl;

      this->pre_exit = false;
      this->exit     = false;
      return true;
    }
  }
  return false;
}



/* ########################################################################################
######   ExecuteCommand(string command)                                              ######
######                                                                               ######
######   Version: 10.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) command   : a string containing a command to be executed                 ######
######   output:                                                                     ######
######   return value : command executed?                                            ######
###########################################################################################
######   description:                                                                ######
######   Interprets "command" and executes it. If "command" is not a system command  ######
######   call "ExecuteOrbifoldCommand(command)".                                     ######
######################################################################################## */
bool CPrompt::ExecuteCommand(string command)
{
  string parameter_string1 = "";
  string parameter_string2 = "";

  // ignore comments
  // updated on 17.09.2010
  if (this->FindCommandType1(command, "//", parameter_string1))
    return true;

  if (this->FindCommandType1(command, "@endif", parameter_string1))
    return true;

  if (this->FindCommandType1(command, "@else", parameter_string1))
    return true;

  // print enter
  // updated on 20.04.2011
  if (this->FindCommandType1(command, "@print enter", parameter_string1))
  {
    (*this->Print.out) << endl;

    if (this->print_output)
      this->MessageParameterNotKnown(parameter_string1);
    return true;
  }

  // print line
  // updated on 20.04.2011
  if (this->FindCommandType2(command, "@print(", parameter_string1, parameter_string2))
  {
    if (this->FindParameterType1(parameter_string2, "unformatted"))
      (*this->Print.out) << parameter_string1;
    else
      (*this->Print.out) << this->Print.cbegin << parameter_string1 << this->Print.cend << endl;

    if (this->print_output)
      this->MessageParameterNotKnown(parameter_string2);
    return true;
  }

  bool restore_old_output_type = false;
  OutputType old_output_type = Tstandard;

  if (this->FindParameterType1(command, "@mathematica"))
  {
    restore_old_output_type = true;
    old_output_type = this->Print.GetOutputType();
    this->Print.SetOutputType(Tmathematica);
  }
  else
  if (this->FindParameterType1(command, "@latex"))
  {
    restore_old_output_type = true;
    old_output_type = this->Print.GetOutputType();
    this->Print.SetOutputType(Tlatex);
  }
  else
  if (this->FindParameterType1(command, "@standard"))
  {
    restore_old_output_type = true;
    old_output_type = this->Print.GetOutputType();
    this->Print.SetOutputType(Tstandard);
  }

  bool     cend_printed = false;
  unsigned DurationWait = 5;

  size_t t1 = 0;
  size_t t2 = 0;
  size_t t3 = 0;
  unsigned i = 0;
  unsigned j = 0;
  unsigned k = 0;

  string tmp_string1 = "";
  string tmp_string2 = "";

  // step 1: child processes
  // step 2: prompt commands

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // wait until all child processes are finished
  // updated on 20.04.2011
  if (this->FindCommandType2(command, "wait(", parameter_string1, parameter_string2) || this->FindCommandType1(command, "wait", parameter_string2))
  {
    const unsigned waiting_intervall = 5;
    const unsigned MAX_waiting_intervall = 600;

    DurationWait = waiting_intervall;

    if (parameter_string1.size() != 0)
    {
      if (parameter_string1.find_first_not_of("0123456789") != string::npos)
      {
        if (this->print_output)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Duration of waiting interval ill defined: \"" << parameter_string1 << "\". Set to " << waiting_intervall << " sec." << this->Print.cend << "\n" << endl;

        DurationWait = waiting_intervall;
      }
      else
      {
        DurationWait = (unsigned)atoi(parameter_string1.c_str());
        if (DurationWait > MAX_waiting_intervall)
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Duration of waiting interval too long. Set to " << MAX_waiting_intervall << " sec." << this->Print.cend << "\n" << endl;

          DurationWait = MAX_waiting_intervall;
        }
      }
    }

    if (this->print_output)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "Waiting for child processes to finish..." << flush;

    sleep(1);
    this->wait_for_processes = true;
  }

  CAnalyseModel Analyse;

  // STEP 1 //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // begin: child processes
  t1 = this->Orbifolds.size();

  bool NoProcessRunning = true;
  do {
    NoProcessRunning = true;

    // begin: run through all orbifold directories of the prompt
    for (i = 0; i < t1; ++i)
    {
      COrbifold       &Orbifold   = this->Orbifolds[i];
      vector<SConfig> &VEVConfigs = this->AllVEVConfigs[i];

      // begin: run through all vev-configurations of the current orbifold directory
      t2 = VEVConfigs.size();
      for (j = 0; j < t2; ++j)
      {
        SConfig &VEVConfig = VEVConfigs[j];

        PID &PID_Data = VEVConfig.pid;
        const size_t p1 = PID_Data.PIDs.size();

        // begin: check status of child processes
        for (k = 0; k < p1; ++k)
        {
          if (!PID_Data.PID_Done[k])
          {
            pid_t currentPID = PID_Data.PIDs[k];

            int status = 0;
            if (waitpid(currentPID, &status, WNOHANG) > 0)
            {
              PID_Data.PID_Done[k] = true;

              std::ostringstream osID;
              osID << currentPID;
              const  string   Filename = PID_Data.PID_Filenames[k];
              const  unsigned JobIndex = PID_Data.PID_JobIndices[k];
              string          Command  = PID_Data.PID_Commands[k];

              const double diff = difftime (time(NULL), PID_Data.PID_StartingTimes[k]);
              // begin: process finished without problems
              if (WIFEXITED(status))
              {
                if (this->wait_for_processes)
                {
                  if (!cend_printed)
                    (*this->Print.out) << this->Print.cend << endl;
                  cend_printed = true;
                }

                if (i == this->OrbifoldIndex)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "PID " << currentPID << " done: \"" << Command << "\" TIME: " << setfill ('0') << setw (2) << (unsigned)(diff/3600.0) << ":" << setw (2) << (unsigned)(fmod((diff/60.0),60.0)) << ":" << setw (2) << (unsigned)fmod(diff, 60.0) << setfill (' ') << this->Print.cend << "\n" << endl;
                else
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "For orbifold \"" << Orbifold.OrbifoldGroup.Label << "\" PID " << currentPID << " done: \"" << Command << "\" TIME: " << setfill ('0') << setw (2) << (unsigned)(diff/3600.0) << ":" << setw (2) << (unsigned)(fmod((diff/60.0),60.0)) << ":" << setw (2) << (unsigned)fmod(diff, 60.0) << setfill (' ') << this->Print.cend << "\n" << endl;

                vector<CField> &Fields = VEVConfig.Fields;

                std::ifstream in;
                if (JobIndex != 3)
                {
                  in.open(Filename.data());
                  if((!in.is_open()) || (!in.good()))
                  {
                    (*this->Print.out) << "\n  " << this->Print.cbegin << "File \"" << Filename << "\" not found." << this->Print.cend << "\n" << endl;
                    return true;
                  }
                }

                // updated on 01.07.2011
                if (JobIndex == 0) // create couplings
                {
                  const unsigned counter = Orbifold.YukawaCouplings.Load(Orbifold, VEVConfig, in);
                  (*this->Print.out) << "  " << this->Print.cbegin << counter << " couplings created." << this->Print.cend;
                  if (counter != 0)
                  {
                    t1 = VEVConfig.MassMatrices.size();
                    if ((t1 != 0) && this->print_output)
                      (*this->Print.out) << "\n  " << this->Print.cbegin << "Updating the mass matrices.";

                    for (unsigned l = 0; l < t1; ++l)
                    {
                      CMassMatrix &Test_MassMatrix = VEVConfig.MassMatrices[l];
                      Test_MassMatrix.Update(VEVConfig);
                      if (this->print_output)
                        (*this->Print.out) << "." << flush;
                    }

                    if ((t1 != 0) && this->print_output)
                      (*this->Print.out) << "done." << this->Print.cend;
                  }
                  (*this->Print.out) << "\n" << endl;
                  string rm_command1 = "rm " + Filename;
                  if (system(rm_command1.data()) != 0)
                    cout << "  System(" << rm_command1 << ") failed." << endl;

                  string rm_command2 = "rm tmp_combinations_ID" + osID.str() + ".txt";
                  if (system(rm_command2.data()) != 0)
                    cout << "  System(" << rm_command2 << ") failed." << endl;
                }
                else
                if (JobIndex == 1) // D-flat
                {
                  vector<unsigned> AllFieldsInMonomial;

                  string currentline = "";
                  (*this->Print.out) << endl;
                  while (getline(in, currentline))
                  {
                    if (currentline.substr(0,7) == "begin: ")
                    {
                      if (currentline == "begin: print")
                      {
                        while (getline(in, currentline) && (currentline != "end: print"))
                          (*this->Print.out) << currentline << endl;
                      }
                      else
                      if (currentline == "begin: AllFieldsInMonomial")
                      {
                        unsigned temp = 0;
                        while (getline(in, currentline) && (currentline != "end: AllFieldsInMonomial"))
                        {
                          std::istringstream line(currentline);
                          while (line >> temp) AllFieldsInMonomial.push_back(temp);
                        }
                      }
                      if (currentline == "begin: Monomials")
                      {
                        vector<string>    &NamesOfMonomials = VEVConfig.NamesOfMonomials;
                        vector<CMonomial> &SetOfMonomials   = VEVConfig.SetOfMonomials;

                        const size_t m1 = NamesOfMonomials.size();
                        const size_t NumberOfU1s = VEVConfig.SymmetryGroup.GaugeGroup.u1directions.size();

                        unsigned counter = m1;

                        while (GetSaveLine(in, currentline) && (currentline != "end: Monomials"))
                        {
                          ++counter;
                          tmp_string1 = "Mon";
                          std::ostringstream os;
                          os << counter;
                          tmp_string1 += os.str();

                          CMonomial NewMonomial(NumberOfU1s);
                          if (NewMonomial.ReadMonomial(currentline, Fields, VEVConfig.use_Labels))
                          {
                            NamesOfMonomials.push_back(tmp_string1);
                            SetOfMonomials.push_back(NewMonomial);
                          }
                          else
                            (*this->Print.out) << "Warning: could not load monomial from string: \"" << currentline << "\"." << endl;
                        }
                        const size_t m2 = NamesOfMonomials.size();

                        (*this->Print.out) << "\n  " << this->Print.cbegin;
                        switch(counter)
                        {
                          case 0:
                            (*this->Print.out) << "No monomials found."; break;
                          case 1:
                            (*this->Print.out) << "One monomial found and saved as: " << NamesOfMonomials[m2-1]; break;
                          default:
                          {
                            (*this->Print.out) << counter << " monomials found and saved as: " << flush;
                            if (diff < 4)
                            {
                              for (unsigned m = m1; m < m2; ++m)
                                (*this->Print.out) << NamesOfMonomials[m] << " ";
                            }
                            else
                              (*this->Print.out) << NamesOfMonomials[m1] << " ... " << NamesOfMonomials[m2-1];
                            break;
                          }
                        }
                        (*this->Print.out) << this->Print.cend << "\n";
                      }
                    }
                  }

                  // begin: save AllFieldsInMonomial to a set
                  if (this->FindParameterType2(Command, "save to set(", tmp_string1))
                  {
                    vector<string>            &NamesOfSetsOfFields = VEVConfig.NamesOfSetsOfFields;
                    vector<vector<unsigned> > &SetsOfFields        = VEVConfig.SetsOfFields;

                    if (this->MessageLabelError(tmp_string1) || this->MessageXAlreadyExists(NamesOfSetsOfFields, "Set", tmp_string1))
                      return true;

                    (*this->Print.out) << "\n  " << this->Print.cbegin << "Result saved to " << flush;
                    vector<string>::iterator pos = find(NamesOfSetsOfFields.begin(), NamesOfSetsOfFields.end(), tmp_string1);
                    if (pos == NamesOfSetsOfFields.end())
                    {
                      (*this->Print.out) << "new" << flush;
                      NamesOfSetsOfFields.push_back(tmp_string1);

                      stable_sort(AllFieldsInMonomial.begin(), AllFieldsInMonomial.end());
                      SetsOfFields.push_back(AllFieldsInMonomial);
                    }
                    else
                    {
                      (*this->Print.out) << "existing" << flush;
                      vector<unsigned> &CurrentSet = SetsOfFields[distance(NamesOfSetsOfFields.begin(), pos)];

                      // add only those fields that are not contained in the current set yet
                      unsigned tmp = 0;
                      const size_t t1 = AllFieldsInMonomial.size();
                      for (unsigned l = 0; l < t1; ++l)
                      {
                        tmp = AllFieldsInMonomial[l];
                        if (find(CurrentSet.begin(), CurrentSet.end(), tmp) == CurrentSet.end())
                          CurrentSet.push_back(tmp);
                      }
                      stable_sort(CurrentSet.begin(), CurrentSet.end());
                    }
                    (*this->Print.out) << " set named " << tmp_string1 << "." << this->Print.cend << "\n";
                  }
                  (*this->Print.out) << endl;
                  // end: save AllFieldsInMonomial to a set
                  string rm_command = "rm " + Filename;
                  if (system(rm_command.data()) != 0)
                    cout << "  System(" << rm_command << ") failed." << endl;
                }
                else
                if (JobIndex == 2) // find accidental U(1) symmetries
                {
                  string currentline = "";
                  getline(in, currentline);
                  if (currentline == "begin: print")
                  {
                    while (getline(in, currentline) && (currentline != "end: print"))
                      (*this->Print.out) << currentline << endl;
                  }
                  getline(in, currentline);
                  if (currentline == "begin: Accidental U1 charges")
                  {
                    if (Analyse.AccidentalU1Charges_Load(VEVConfig, in))
                    {
                      if (this->print_output)
                        (*this->Print.out) << "\n  " << this->Print.cbegin << "Accidental U(1) charges load." << this->Print.cend << "\n" << endl;
                    }
                    else
                    {
                      if (this->print_output)
                        (*this->Print.out) << "\n  " << this->Print.cbegin << "Warning: Cannot load accidental U(1) charges." << this->Print.cend << "\n" << endl;
                    }
                  }
                  string rm_command = "rm " + Filename;
                  if (system(rm_command.data()) != 0)
                    cout << "  System(" << rm_command << ") failed." << endl;
                }

                if (JobIndex != 3)
                  in.close();
              }
              // end: process finished without problems
              else
                (*this->Print.out) << "\n  " << this->Print.cbegin << "PID " << currentPID << " stopped: \"" << PID_Data.PID_Commands[k] << "\" TIME: " << setfill ('0') << setw (2) << (unsigned)(diff/3600.0) << ":" << setw (2) << (unsigned)(fmod((diff/60.0),60.0)) << ":" << setw (2) << (unsigned)fmod(diff, 60.0) << setfill (' ') << this->Print.cend << "\n" << endl;

              // updated on 24.08.2011
              if (JobIndex == 3) // create random orbifold
              {
                if (this->FindParameterType1(Command, "load when done"))
                  this->LoadOrbifolds(Filename, false, 0);

                if (!this->FindParameterType2(Command, "to(", tmp_string1))
                {
                  string rm_command = "rm " + Filename;
                  if (system(rm_command.data()) != 0)
                    cout << "  System(" << rm_command << ") failed." << endl;
                }
                else
                  (*this->Print.out) << "  " << this->Print.cbegin << "New orbifolds saved to file \"" << Filename << "\"." << this->Print.cend << "\n" << endl;
              }
            }
          }
        }
        // end: check status of child processes

        if (this->wait_for_processes && (find(PID_Data.PID_Done.begin(), PID_Data.PID_Done.end(), false) != PID_Data.PID_Done.end()))
          NoProcessRunning = false;
      }
      // end: run through all vev-configurations of the current orbifold directory
    }
    // end: run through all orbifold directories of the prompt

    if (this->wait_for_processes)
    {
      if (NoProcessRunning)
      {
        if (this->print_output)
        {
          if (cend_printed)
            (*this->Print.out) << "  " << this->Print.cbegin;
          (*this->Print.out) << "waiting done." << this->Print.cend << endl;
        }
        this->wait_for_processes = false;
        sleep(1);

        this->MessageParameterNotKnown(parameter_string2);
        return true;
      }
      else
      {
        if (this->print_output)
          (*this->Print.out) << "." << flush;

        sleep(DurationWait);
      }
    }
  } while(this->wait_for_processes);
  // end: child processes
  // STEP 1 //////////////////////////////////////////////////////////////////////////////////////////////////////////


  // STEP 2 //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // begin: prompt commands

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // kill a processes
  // updated on 14.07.2011
  if (this->FindCommandType2(command, "kill(", parameter_string1, parameter_string2))
  {
    const bool KillAll = (parameter_string1 == "all");

    if (!KillAll && (parameter_string1.find_first_not_of("0123456789") != string::npos))
    {
      if (this->print_output)
      {
        (*this->Print.out) << "\n  " << this->Print.cbegin << "PID \"" << parameter_string1 << "\" ill defined." << this->Print.cend << "\n" << endl;
        this->MessageParameterNotKnown(parameter_string2);
      }
      return true;
    }


    pid_t killPID = 0;
    if (!KillAll)
      killPID = (unsigned)atoi(parameter_string1.c_str());

    bool PID_found = false;
    if (KillAll)
      PID_found = true;

    vector<unsigned> IndicesOfProcessesToKill;

    t1 = this->AllVEVConfigs.size();
    for (i = 0; i < t1; ++i)
    {
      vector<SConfig> &VEVConfigs = this->AllVEVConfigs[i];

      t2 = VEVConfigs.size();
      for (j = 0; j < t2; ++j)
      {
        PID &PID_Data = VEVConfigs[j].pid;

        IndicesOfProcessesToKill.clear();
        if (KillAll)
        {
          t3 = PID_Data.PIDs.size();
          for (k = 0; k < t3; ++k)
            IndicesOfProcessesToKill.push_back(k);
        }
        else
        {
          vector<pid_t>::iterator pos = find(PID_Data.PIDs.begin(), PID_Data.PIDs.end(), killPID);
          if (pos != PID_Data.PIDs.end())
          {
            PID_found = true;
            IndicesOfProcessesToKill.push_back(distance(PID_Data.PIDs.begin(), pos));
          }
        }

        t3 = IndicesOfProcessesToKill.size();
        if ((t3 != 0) && this->print_output)
          (*this->Print.out) << "\n";

        for (k = 0; k < t3; ++k)
        {
          if (!PID_Data.PID_Done[IndicesOfProcessesToKill[k]])
          {
            string command = "kill ";
            std::ostringstream os;
            os << PID_Data.PIDs[IndicesOfProcessesToKill[k]];
            command += os.str();

            if (system(command.data()) != 0)
              cout << "  System(" << command << ") failed." << endl;

            if (this->print_output)
              (*this->Print.out) << "  " << this->Print.cbegin << "Executing command \"" << command << "\"." << this->Print.cend << "\n";
          }
        }
      }
    }

    if (!PID_found && this->print_output)
    {
      (*this->Print.out) << "\n  " << this->Print.cbegin << "PID " << killPID << " unknown." << this->Print.cend << "\n" << endl;
      this->MessageParameterNotKnown(parameter_string2);
    }

    if (this->print_output)
    {
      (*this->Print.out) << endl;
      this->MessageParameterNotKnown(parameter_string2);
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // show active processes
  // updated on 11.05.2011
  if (this->FindCommandType1(command, "ps", parameter_string1))
  {
    double diff = 0.0;
    bool printed = false;

    (*this->Print.out) << "\n";
    t1 = this->Orbifolds.size();
    for (i = 0; i < t1; ++i)
    {
      const COrbifold       &Orbifold   = this->Orbifolds[i];
      const vector<SConfig> &VEVConfigs = this->AllVEVConfigs[i];

      t2 = VEVConfigs.size();
      for (j = 0; j < t2; ++j)
      {
        const SConfig &VEVConfig = VEVConfigs[j];
        const PID     &PID_Data  = VEVConfig.pid;

        if (find(PID_Data.PID_Done.begin(), PID_Data.PID_Done.end(), false) != PID_Data.PID_Done.end())
        {
          printed = true;
          (*this->Print.out) << "  " << this->Print.cbegin << "Processes of model \"" << Orbifold.OrbifoldGroup.Label << "\" in vev-configuration \"" << VEVConfig.ConfigLabel << VEVConfig.ConfigNumber <<  "\":" << this->Print.cend << "\n";
          (*this->Print.out) << "  " << this->Print.cbegin << "  PID       TIME CMD" << this->Print.cend << endl;
          t3 = PID_Data.PIDs.size();
          for (k = 0; k < t3; ++k)
          {
            if (!PID_Data.PID_Done[k])
            {
              diff = difftime (time(NULL), PID_Data.PID_StartingTimes[k]);
              (*this->Print.out) << "  " << this->Print.cbegin << setw(5) << PID_Data.PIDs[k] << "   ";
              (*this->Print.out) << setfill ('0') << setw(2) << (unsigned)(diff/3600.0) << ":" << setw(2) << (unsigned)(fmod((diff/60.0),60.0)) << ":" << setw(2) << (unsigned)fmod(diff, 60.0);
              (*this->Print.out) << setfill (' ') << " " << PID_Data.PID_Commands[k] << this->Print.cend << "\n";
            }
          }
          (*this->Print.out) << endl;
        }
      }
    }
    if (!printed)
      (*this->Print.out) << "  " << this->Print.cbegin << "No processes are running." << this->Print.cend << "\n" << endl;

    this->MessageParameterNotKnown(parameter_string1);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // exit orbifolder
  // updated on 12.10.2011
  if (this->FindCommandType1(command, "exit orbifolder", parameter_string1))
  {
    this->exit = true;
    this->MessageParameterNotKnown(parameter_string1);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // exit
  // updated on 12.10.2011
  if (!this->online_mode && this->FindCommandType1(command, "exit", parameter_string1))
  {
    this->pre_exit = true;

    t1 = this->AllVEVConfigs.size();
    for (i = 0; this->pre_exit && (i < t1); ++i)
    {
      const vector<SConfig> &VEVConfigs = this->AllVEVConfigs[i];

      t2 = VEVConfigs.size();
      for (j = 0; this->pre_exit && (j < t2); ++j)
      {
        const PID &PID_Data = VEVConfigs[j].pid;

        if (find(PID_Data.PID_Done.begin(), PID_Data.PID_Done.end(), false) != PID_Data.PID_Done.end())
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Cannot exit - some processes are stil running." << this->Print.cend << "\n" << endl;

          this->pre_exit = false;
        }
      }
    }
    if (this->pre_exit)
    {
      if (this->print_output)
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Do you really want to quit? Type \"yes\" to quit or \"no\" to continue." << this->Print.cend << "\n" << endl;
    }
    this->MessageParameterNotKnown(parameter_string1);
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // system commands
  // updated on 20.04.2011
  if (this->FindCommandType1(command, "@", parameter_string1))
  {
    if (!this->online_mode && this->FindParameterType1(parameter_string1, "end print to file"))
    {
      this->output_filename     = "";
      this->keep_output_to_file = false;
      this->Print.out = &cout;
    }
    else
    if (this->FindParameterType1(parameter_string1, "status"))
    {
      (*this->Print.out) << "\n  status:\n";
      if (this->online_mode)
        (*this->Print.out) << "    output is written to the web interface.\n";
      else
      {
        (*this->Print.out) << "    output is written to: ";
        if (this->keep_output_to_file)
          (*this->Print.out) << "file \"" << this->output_filename << "\".\n";
        else
          (*this->Print.out) << "screen.\n";
      }

      (*this->Print.out) << "    typesetting: ";
      if (this->Print.GetOutputType() == Tstandard)
        (*this->Print.out) << "standard\n";
      if (this->Print.GetOutputType() == Tmathematica)
        (*this->Print.out) << "mathematica\n";
      if (this->Print.GetOutputType() == Tlatex)
        (*this->Print.out) << "latex\n";
      (*this->Print.out) << endl;
    }
    else
    if (this->FindParameterType2(parameter_string1, "typesetting(", tmp_string1))
    {
      if (tmp_string1 == "mathematica")
      {
        this->Print.SetOutputType(Tmathematica);

        if (this->print_output)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Typesetting set to mathematica style." << this->Print.cend << "\n" << endl;
      }
      else
      if (tmp_string1 == "latex")
      {
        this->Print.SetOutputType(Tlatex);

        if (this->print_output)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Typesetting set to latex style." << this->Print.cend << "\n" << endl;
      }
      else
      if (tmp_string1 == "standard")
      {
        this->Print.SetOutputType(Tstandard);

        if (this->print_output)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Typesetting set to standard style." << this->Print.cend << "\n" << endl;
      }
    }
    this->MessageParameterNotKnown(parameter_string1);
    return true;
  }
  // end: prompt commands
  // STEP 2 //////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  this->ExecuteOrbifoldCommand(command);
  
  if (restore_old_output_type)
  {
    restore_old_output_type = false;
    this->Print.SetOutputType(old_output_type);
  }
  return true;
}



/* ########################################################################################
######   ExecuteOrbifoldCommand(string command)                                      ######
######                                                                               ######
######   Version: 12.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) command   : a string containing a command to be executed                 ######
######   output:                                                                     ######
######   return value : command executed?                                            ######
###########################################################################################
######   description:                                                                ######
######   Interprets "command" and executes it.                                       ######
######################################################################################## */
bool CPrompt::ExecuteOrbifoldCommand(string command)
{ // a begin
  string tmp_string1 = "";
  string tmp_string2 = "";
  string tmp_string3 = "";
  string parameter_string1 = "";
  string parameter_string2 = "";
  string parameter_string3 = "";
  size_t t1 = 0;
  size_t t2 = 0;
  unsigned i = 0;
  unsigned j = 0;
  unsigned k = 0;

  bool case1 = false;
  bool case2 = false;

  CAnalyseModel Analyse;

  // step 6: child processes
  // step 4: commands available in all directories
  // step 5: commands available in specific folders


  // if the current directory is />
  if (this->current_folder[0] < 0)
  { //b  begin
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // change directory
    // updated on 10.10.2011
    if (this->FindCommandType1(command, "cd ", parameter_string1))
    {
      unsigned index = 0;
      if (this->MessageOrbifoldNotKnown(parameter_string1, index))
        return true;

      this->OrbifoldIndex = index;
      this->current_folder[0] = index;

      const COrbifold &Orbifold = this->Orbifolds[this->OrbifoldIndex];
      if (Orbifold.GetCheckStatus() == CheckedAndGood)
      {
        const SelfDualLattice Lattice = Orbifold.OrbifoldGroup.GetLattice();

        if (Lattice == E8xE8)
          SELFDUALLATTICE = 1;
        /*else
        if (Lattice == Spin32)
          SELFDUALLATTICE = 2;
        else
        {
          SELFDUALLATTICE = 1;
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Self-dual lattice not defined. Set to E8xE8." << this->Print.cend << "\n" << endl;
        }*/
        return true;
      }

      if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
        this->MessageHelpCreateNewOrbifold(3);
      else
        this->MessageHelpCreateNewOrbifold();
      this->current_folder[1] = 1;
      return true;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    
    // rename orbifold    //sept16
    if (this->FindCommandType2(command, "rename orbifold(", parameter_string1, parameter_string2))
    {
      // begin: find parameters
      if (!this->FindParameterType2(parameter_string2, "to(", tmp_string1))
      {
        if (this->print_output)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "New label of orbifold not specified." << this->Print.cend << "\n" << endl;

        this->MessageParameterNotKnown(parameter_string2);
        return true;
      }
      // end: find parameters

      // begin: from orbifold
      unsigned index = 0;
      if (this->MessageOrbifoldNotKnown(parameter_string1, index))
        return true;
      // end: from orbifold

      // begin: to orbifold
      if (this->MessageLabelError(tmp_string1))
      {
        this->MessageParameterNotKnown(parameter_string2);
        return true;
      }

      if (this->MessageOrbifoldAlreadyExists(tmp_string1))
      {
        if (!this->MessageParameterNotKnown(parameter_string2))
          (*this->Print.out) << endl;
        return true;
      }
      // end: to orbifold

      this->Orbifolds[index].OrbifoldGroup.Label = tmp_string1;

      if (this->print_output)
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold \""  << parameter_string1 << "\" renamed to \"" << tmp_string1 << "\"." << this->Print.cend << "\n" << endl;

      this->MessageParameterNotKnown(parameter_string2);
      return true;
    }    
    
    
//begin ADD from U1s
// the original laod orbifolds before april 26
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // load orbifolds 
/*    if (this->FindCommandType2(command, "load orbifolds(", parameter_string1, parameter_string2)
     || this->FindCommandType2(command, "load orbifold(", parameter_string1, parameter_string2))  //sept16
    {
      (*this->Print.out) << "\n";
      this->LoadOrbifolds(parameter_string1, false, false);  //tambien ok deleting false,false
      this->MessageParameterNotKnown(parameter_string2);
      return true;
    }
*/
//end ADD from U1s
/////////////////////////////////
// begin new load orbifolds, april 26

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // load orbifolds 
    if (this->FindCommandType2(command, "load orbifolds(", parameter_string1, parameter_string2)
     || this->FindCommandType2(command, "load orbifold(", parameter_string1, parameter_string2))  //sept16
    {
      const bool inequivalent      = this->FindParameterType1(parameter_string2, "inequivalent");
      (*this->Print.out) << "\n";
      this->LoadOrbifolds(parameter_string1, inequivalent, false);  //tambien ok deleting false,false
      this->MessageParameterNotKnown(parameter_string2);
      return true;
    }

///// end new loas orbifolds, april 26
/////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // save orbifolds //sept16
    if (this->FindCommandType2(command, "save orbifolds(", parameter_string1, parameter_string2)
     || this->FindCommandType2(command, "save orbifold(", parameter_string1, parameter_string2))
    {
      t1 = this->Orbifolds.size();
      if (t1 == 0)
      {
        if (this->print_output)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "No orbifold to save." << this->Print.cend << "\n" << endl;
        this->MessageParameterNotKnown(parameter_string2);
        return true;
      }

      ofstream out(parameter_string1.data());
      if((!out.is_open()) || (!out.good()))
      {
        (*this->Print.out) << "\n  " << this->Print.cbegin << "File \"" << parameter_string1 << "\" not found." << this->Print.cend << "\n" << endl;

        this->MessageParameterNotKnown(parameter_string2);
        return true;
      }

      unsigned counter = 0;
      for (i = 0; i < t1; ++i)
      {
        if (this->Orbifolds[i].GetCheckStatus() == CheckedAndGood)
        {
          this->Orbifolds[i].OrbifoldGroup.PrintToFile(out);
          out << endl;
          ++counter;
        }
      }
      out.close();

      if (this->print_output)
      {
        if (counter == 0)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "No orbifolds saved to file \"" << parameter_string1 << "\"." << this->Print.cend << "\n" << endl;
        else
        if (counter == 1)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "One orbifold saved to file \"" << parameter_string1 << "\"." << this->Print.cend << "\n" << endl;
        else
          (*this->Print.out) << "\n  " << this->Print.cbegin << counter << " orbifolds saved to file \"" << parameter_string1 << "\"." << this->Print.cend << "\n" << endl;
      }

      this->MessageParameterNotKnown(parameter_string2);
      return true;
    } 

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // create orbifold
    // updated on 21.10.2011
    if (this->FindCommandType2(command, "create orbifold(", parameter_string1, parameter_string2))
    {
      
      const bool b2 = this->FindParameterType2(parameter_string2, "with point group(", parameter_string3);

      // case 2: "with point group(M,N)"
      vector<int> Orders;
      
  if (!convert_string_to_vector_of_int(parameter_string3, Orders) || ((Orders.size() != 1) && (Orders.size() != 2)) || (find(Orders.begin(), Orders.end(), 0) != Orders.end()))  //N1orig (couls also be used in N0)
  
//  if (!convert_string_to_vector_of_int(parameter_string3, Orders) || ((Orders.size() != 1) && (Orders.size() != 2) && (Orders.size() != 3)) || (find(Orders.begin(), Orders.end(), 0) != Orders.end()))  //N0added
  
      {
        if (this->print_output)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "The point group of the orbifold is ill-defined." << this->Print.cend << "\n" << endl;

        this->MessageParameterNotKnown(parameter_string2);
        return true;
      }

/*      int ZM = 1;     //N1orig
      int ZN = 1;
      if (Orders.size() == 1)
        ZM = Orders[0];
      else
      {
        ZM = Orders[0];
        ZN = Orders[1];
      }
*/      
 
 /////////////
 
   // begin try for N0
       int ZM = 2;  
       int ZN = 1;
       int ZK = 1;
      if (Orders.size() == 1)
       {
       ZN = Orders[0];
       }   
      else
      {
        ZN = Orders[0];
        ZK = Orders[1];   
      }
      // end try for N0
  
 ///////////    

 /////////// begin try for N0
/*       int ZM = 1;  
       int ZN = 1;
       int ZK = 1;
      if (Orders.size() == 1)
       {
	   ZM = Orders[0];
       ZN = Orders[1];
       }   
      else
      {
        ZM = Orders[0];
        ZN = Orders[1];
        ZK = Orders[2];   
      } */
 ////////// end try for N0
       
      COrbifold NewOrbifold;
      CSpaceGroup &SpaceGroup = NewOrbifold.OrbifoldGroup.AccessSpaceGroup();
      SpaceGroup.Clear();
      //SpaceGroup.SetOrder(ZM, ZN);   //N1 orig
      SpaceGroup.SetOrder(ZM, ZN, ZK);  // try for N0
      
      
      NewOrbifold.OrbifoldGroup.Label = parameter_string1;
      this->Orbifolds.push_back(NewOrbifold);

      SConfig TestConfig = NewOrbifold.StandardConfig;
      TestConfig.ConfigLabel = "TestConfig";
      TestConfig.ConfigNumber = 1;

      vector<SConfig> Configs;
      Configs.push_back(NewOrbifold.StandardConfig);
      Configs.push_back(TestConfig);
      this->AllVEVConfigs.push_back(Configs);
      this->AllVEVConfigsIndex.push_back(1);

/*      if (this->print_output)
      {
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold with point group Z" << SpaceGroup.GetM();
        if (SpaceGroup.IsZMxZN())
          (*this->Print.out) << "xZ" << SpaceGroup.GetN();
        (*this->Print.out) << " created and stored in directory \"" << parameter_string1 << "\"." << this->Print.cend << "\n" << endl;
        (*this->Print.out) << "  " << this->Print.cbegin << "Use the command \"cd " << parameter_string1 << "\" to change the directory to the new orbifold." << this->Print.cend << "\n" << endl;
      }
*/

     if (this->print_output)
      {
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold with point group "; //Z" << SpaceGroup.GetM();
        if (SpaceGroup.IsZMxZN())
          (*this->Print.out) << "Z" << SpaceGroup.GetN();
        if (SpaceGroup.IsZMxZNxZK())
          (*this->Print.out) << "xZ" << SpaceGroup.GetK();          
          
        (*this->Print.out) << " created and stored in directory \"" << parameter_string1 << "\"." << this->Print.cend << "\n" << endl;
        
       // (*this->Print.out) << " ( Z2 is just used for breaking internally E8xE8 to SO(16)xSO(16) )." << this->Print.cend << "\n" << endl;  //added J27
        
        (*this->Print.out) << "  " << this->Print.cbegin << "Use the command \"cd " << parameter_string1 << "\" to change the directory to the new orbifold." << this->Print.cend << "\n" << endl;
      }


      this->MessageParameterNotKnown(parameter_string2);
      return true;
    }
    
    
//**********BEGIN CR  Try v2ok-short Sept6
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // create random orbifold
    // updated on 22.02.2012
    if (this->FindCommandType2(command, "create random orbifold from(", parameter_string1, parameter_string2))
    {
 // begin: find parameters

      bool save_if_new = false; //addedA sept6

      bool save_all    = true;  //sept7
      bool save_SM     = false;
      bool save_PS     = false;
      bool save_SU5    = false;

      unsigned number_of_generations = 3;
      bool generations_specified = false;  //remove


      bool Use_Filename = false;
      string Filename = "";

      const bool load_when_done    =  this->FindParameterType1(parameter_string2, "load when done");  //added aug30 
      
      
       //beginA added sept6
      
//       if (this->FindParameterType2(parameter_string2, "if(", parameter_string3))
//      {
//        save_if_new = this->FindParameterType1(parameter_string3, "inequivalent");
//      }
      //endA added sept6

            if (this->FindParameterType2(parameter_string2, "if(", parameter_string3))  //sept7
      {
        save_SM     = this->FindParameterType1(parameter_string3, "SM");
        save_PS     = this->FindParameterType1(parameter_string3, "PS");
        save_SU5    = this->FindParameterType1(parameter_string3, "SU5");

        generations_specified = this->FindParameterType4(parameter_string3, "generations", number_of_generations);

        if (generations_specified && (!save_SM && !save_PS && !save_SU5))
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Cannot look for models with " << number_of_generations << " generations, because gauge group not specified." << this->Print.cend << flush;
          generations_specified = false;
        }

        if (save_SM || save_PS || save_SU5)
          save_all = false;

        save_if_new = this->FindParameterType1(parameter_string3, "inequivalent");
      }

      if (this->FindParameterType2(parameter_string2, "save to(", parameter_string3))
      {
        Use_Filename = true;
        Filename = parameter_string3;
      }


      //     const bool create_Orbifold = (!save_all || save_if_new || check_anomalies || print_info); //added sept6
//     const bool create_Orbifold = (save_if_new);  //added sept6
     const bool create_Orbifold = (!save_all || save_if_new); //sept7


      // begin: use original shifts and Wilson lines
//      vector<bool> UseOrigShiftsAndWilsonLines(8, false);  //orig
      // end: use original shifts and Wilson lines


// begin: use original shifts and Wilson lines  // begin pract Aug29
      CRandomModel RandomModel(E8xE8);    //added line (refer below *)
      vector<bool> UseOrigShiftsAndWilsonLines(9, false);   //added line
      UseOrigShiftsAndWilsonLines[0]=true; //use Witten's shift    //added line 

/*      
        UseOrigShiftsAndWilsonLines[1]=true;
        UseOrigShiftsAndWilsonLines[2]=true;
        //UseOrigShiftsAndWilsonLines[3]=false;
        //UseOrigShiftsAndWilsonLines[4]=false;
        UseOrigShiftsAndWilsonLines[5]=true;
        UseOrigShiftsAndWilsonLines[6]=true;
        UseOrigShiftsAndWilsonLines[7]=true;
        UseOrigShiftsAndWilsonLines[8]=true;
*/

      if (this->FindParameterType2(parameter_string2, "use(", parameter_string3))
      {
        vector<string>   tmp_strings;
        vector<unsigned> tmp_unsigneds;
        global_DecomposeString(parameter_string3, tmp_strings, ",");
        global_ConvertVectorOfString2VectorOfUnsigned(tmp_strings, tmp_unsigneds);

        bool vector_ok = true;
        if (tmp_unsigneds.size() != 8)
          vector_ok = false;
        for (i = 0; vector_ok && (i < 8); ++i)
        {
          if ((tmp_unsigneds[i] != 0) && (tmp_unsigneds[i] != 1))
            vector_ok = false;
        }
        bool all_entries_one = true;
        for (i = 0; all_entries_one && (i < 8); ++i)
        {
          if (tmp_unsigneds[i] != 1)
            all_entries_one = false;
        }
        if (all_entries_one)
          vector_ok = false;

        if (!vector_ok)
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Parameter \"" << parameter_string3 << "\" of \"use(...)\" ill-defined." << this->Print.cend << "\n" << endl;
          return true;
        }

        for (i = 0; i < 8; ++i)
          UseOrigShiftsAndWilsonLines[i+1] = (tmp_unsigneds[i] == 1);   //UseOrigShiftsAndWilsonLines[i] in N=1 
      }
// end: use original shifts and Wilson lines   // end pract Aug29


      unsigned max_models = 1;  //1 (uno orig), Aug29
//////////////***** begin added Aug30 ok
      if (this->FindParameterType2(parameter_string2, "#models(", parameter_string3))
      {
        if (parameter_string3 == "all")
          max_models = 250000000;
        else
        {
          if (parameter_string3.find_first_not_of("0123456789") != string::npos)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Parameter \"" << parameter_string3 << "\" of \"#models(X)\" ill-defined." << this->Print.cend << "\n" << endl;
            return true;
          }
          max_models = (unsigned)atoi(parameter_string3.c_str());
        }
      }
/////////////**** end added aug30 ok     
      // end: find parameters

      unsigned OriginalOrbifoldIndex = 0;
      vector<unsigned> OrbifoldIndices;

      bool random_origin = false;

//      command += " load when done";  //added Aug30 ok 


//////////begin mult added sept 6

    vector<SUSYMultiplet> Multiplets(2);						//Particle types to be printed
	Multiplets[0]=Scalar;									//and to be given for equivalence check
	Multiplets[1]=LeftFermi;
//////////end mult added sept 6

      int newPID = -1;
      newPID = fork();

      if (newPID < 0)  /* error occurred */
      {
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Fork failed!" << this->Print.cend << "\n" << endl;
        return true;
      }

      if (newPID == 0) /* child process */
      {//k
        
        const double Rmax = RAND_MAX+0.000001;

        std::ofstream tmp_out(Filename.data());

//        CInequivalentModels Models_01;  //aug29
       CInequivalentModels InequivModels; //aded sept6

        COrbifoldGroup NewOrbifoldGroup;
        vector<CVector> UnbrokenRoots;

        vector<CRandomModel>     RandomModels;
        vector<CSector>          CoreSpectrum;
        vector<vector<CSector> > CoreSpectra;

        unsigned SpecialIndex = 0;

        const size_t o1 = OrbifoldIndices.size();
        if (random_origin)
        {
          for (i = 0; i < o1; ++i)
          {
            NewOrbifoldGroup = this->Orbifolds[OrbifoldIndices[i]].OrbifoldGroup;
            NewOrbifoldGroup.LoadedU1Generators.clear();

//orig            CRandomModel RandomModel(NewOrbifoldGroup.GetLattice());
//a29            CRandomModel RandomModel(E8xE8);  //added line but (not) needed here b/c is above*
            RandomModel.Initiate(NewOrbifoldGroup, UseOrigShiftsAndWilsonLines, UnbrokenRoots);

            RandomModels.push_back(RandomModel);

            CoreSpectrum.clear();
            COrbifoldCore OrbifoldCore(NewOrbifoldGroup, CoreSpectrum);
            CoreSpectra.push_back(CoreSpectrum);  
          }
        }
        else
        {
          NewOrbifoldGroup = this->Orbifolds[OriginalOrbifoldIndex].OrbifoldGroup;
          NewOrbifoldGroup.LoadedU1Generators.clear();

//orig          CRandomModel RandomModel(NewOrbifoldGroup.GetLattice());
//aug29           CRandomModel RandomModel(E8xE8);  //added line but (not) needed here b/c is above*
           RandomModel.Initiate(NewOrbifoldGroup, UseOrigShiftsAndWilsonLines, UnbrokenRoots);
           RandomModels.push_back(RandomModel);

          CoreSpectrum.clear();
          COrbifoldCore OrbifoldCore(NewOrbifoldGroup, CoreSpectrum);
          CoreSpectra.push_back(CoreSpectrum);

          SpecialIndex = 0;
        }
       
        vector<SConfig> AllVEVConfigs;

        CSpectrum PrintSpectrum;

        bool save_current_model = false;
        string model_string = "";
        
        bool model_with_problem = false; //added sept6

        unsigned problem_counter = 0;

        i = 1;
        while (i <= max_models)
        { //n

          // create random shifts and Wilson lines
          if (NewOrbifoldGroup.CreateRandom(RandomModels[SpecialIndex], false))
          { //p
            save_current_model = true;
            model_string = "Random";

            if (NewOrbifoldGroup.GetModularInvariance_CheckStatus() != CheckedAndGood)
            {
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Warning: problems with modular invariance." << this->Print.cend << endl;
              model_string += "_ProblemModInv";
            }

//begin if q , added sept6

//           if (create_Orbifold && !model_with_problem)
/*h              if (create_Orbifold)
            {
              COrbifold NewOrbifold(NewOrbifoldGroup, CoreSpectra[SpecialIndex]);

              if (NewOrbifold.GetCheckStatus() != CheckedAndGood)
              {
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Warning: problems with constructing the orbifold spectrum." << this->Print.cend << endl;
                model_with_problem = true;
                model_string += "_ProblemOrbi";
              }

//              if (save_current_model && (save_if_new || print_info) && !model_with_problem)
              if (save_if_new)
              {
//              CSpectrum Spectrum(NewOrbifold.StandardConfig, LeftChiral);
                CSpectrum Spectrum(NewOrbifold.StandardConfig, Multiplets);

                if (save_if_new)
                {
                  
//                  save_current_model = Models_01.IsSpectrumUnknown(Spectrum, true);
                  save_current_model = InequivModels.IsSpectrumUnknown(Spectrum, true);
                }

               
              }

//              if (check_anomalies && !model_with_problem && (!save_if_new || (save_if_new && save_current_model)))
//               if (save_if_new)
//              {
//                if ((!NewOrbifold.CheckAnomaly(NewOrbifold.StandardConfig, this->GaugeIndices, this->Print, false) ))
// //                  || !NewOrbifold.CheckDiscreteAnomaly(NewOrbifold.StandardConfig, this->GaugeIndices, anomalous_element, this->Print, true)))
//                {
//                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Warning: problems with discrete and/or gauge anomalies." << this->Print.cend << endl;
//                  model_with_problem = true;
//                  model_string += "_ProblemAnomaly";
//                }
//              }
            }

*/ //h
//end if q, added sept6

/////// begin if q sept7
//          if (create_Orbifold && !model_with_problem)
          if (create_Orbifold)
            {  // CO
              COrbifold NewOrbifold(NewOrbifoldGroup, CoreSpectra[SpecialIndex]);

              if (NewOrbifold.GetCheckStatus() != CheckedAndGood)
              {
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Warning: problems with constructing the orbifold spectrum." << this->Print.cend << endl;
                model_with_problem = true;
                model_string += "_ProblemOrbi";
              }


             if (NewOrbifold.TachyonicStandardConfig.Fields.size() == 0)	 //oct12
             {//oct12
//              if (!save_all && !model_with_problem)
              if (!save_all)
              {
                AllVEVConfigs.clear();

                bool copy_save_SM  = save_SM;
                bool copy_save_PS  = save_PS;
                bool copy_save_SU5 = save_SU5;
                save_current_model = Analyse.AnalyseModel(NewOrbifold, NewOrbifold.StandardConfig, copy_save_SM, copy_save_PS, copy_save_SU5, AllVEVConfigs, this->Print, number_of_generations, false);
                if (save_current_model)
                {
                  model_string = "Model";
                  if (copy_save_SM)
                    model_string += "_SM";
                  if (copy_save_PS)
                    model_string += "_PS";
                  if (copy_save_SU5)
                    model_string += "_SU5_";
                }
              }
             }  //oct12
             
//              if (save_current_model && (save_if_new || print_info) && !model_with_problem)
              if (save_current_model && save_if_new)            
              {
           //     CSpectrum Spectrum(NewOrbifold.StandardConfig, LeftChiral);
                CSpectrum Spectrum(NewOrbifold.StandardConfig, Multiplets);

                if (save_if_new) 
                {
          //        save_current_model = Models_01.IsSpectrumUnknown(Spectrum, true);
                  save_current_model = InequivModels.IsSpectrumUnknown(Spectrum, true);
                }

              }
            } //CO

/////// end if q sept7

            // begin: save the current model

//            if (save_current_model || model_with_problem)
            if (save_current_model)  
            {
              std::ostringstream os;
              os << i;
              NewOrbifoldGroup.Label = model_string + os.str();
              NewOrbifoldGroup.PrintToFile(tmp_out);
              ++i;
            }
            
            // end: save the current model
          } //p
        } //n end del while

    
       tmp_out.close();

        if (problem_counter == 0)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Models created without problems." << this->Print.cend << flush;
        else
        {
          if (problem_counter == 1)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Warning: 1 model created with problems." << this->Print.cend << flush;
          else
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Warning: " << problem_counter << " models created with problems." << this->Print.cend << flush;
        }

        // terminate child process
        std::exit(0);
      } //k
      /* parent process */
      usleep(100);

      (*this->Print.out) << "\n  " << this->Print.cbegin << "New child process \"PID " << newPID << "\" from command \"" << command << "\"." << this->Print.cend << "\n" << endl;

//begin addedA29

// save info about the process to the orbifold that id the origin of the random models
      PID &PID_Data = this->AllVEVConfigs[OriginalOrbifoldIndex][this->AllVEVConfigsIndex[OriginalOrbifoldIndex]].pid;
      PID_Data.PIDs.push_back(newPID);
      PID_Data.PID_Commands.push_back(command);
      PID_Data.PID_JobIndices.push_back(3); // create random orbifold
      PID_Data.PID_StartingTimes.push_back(time (NULL));
      PID_Data.PID_Done.push_back(false);
      PID_Data.PID_Filenames.push_back(Filename);

//end addedA29

      this->MessageParameterNotKnown(parameter_string2);
      return true;
    }


//***********END CR sept6 

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // delete orbifold  //sept16
    if (this->FindCommandType1(command, "delete orbifold", parameter_string1))
    {
      bool process_running = false;
      if (this->FindParameterType2(parameter_string1, "(", tmp_string1))
      {
        unsigned index = 0;
        if (!this->MessageOrbifoldNotKnown(tmp_string1, index))
        {
          const vector<SConfig> &VEVConfigs = this->AllVEVConfigs[index];
          t1 = VEVConfigs.size(); 
          for (i = 0; !process_running && (i < t1); ++i)
          {
            const vector<bool> &PID_Done = VEVConfigs[i].pid.PID_Done;
            if (find(PID_Done.begin(), PID_Done.end(), false) != PID_Done.end())
              process_running = true;
          }
          if (process_running)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Processes of orbifold \"" << tmp_string1 << "\" are still running. Cannot be deleted." << this->Print.cend << "\n" << endl;
          }
          else
          {
            this->Orbifolds.erase(this->Orbifolds.begin() + index);
            this->AllVEVConfigs.erase(this->AllVEVConfigs.begin() + index);
            this->AllVEVConfigsIndex.erase(this->AllVEVConfigsIndex.begin() + index);
            this->OrbifoldIndex = -1;
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold \"" << tmp_string1 << "\" deleted." << this->Print.cend << "\n" << endl;
          }
        }
      }
      else
      if (this->FindParameterType1(parameter_string1, "s"))
      {
        t1 = this->Orbifolds.size();
        if (t1 == 0)
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "No orbifold to delete." << this->Print.cend << "\n" << endl;
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

        unsigned counter = 0;
        for (int i = t1-1; i >= 0; --i)
        {
          const vector<SConfig> &VEVConfigs = this->AllVEVConfigs[i];
          t2 = VEVConfigs.size(); 

          process_running = false;
          for (j = 0; !process_running && (j < t2); ++j)
          {
            const vector<bool> &PID_Done = VEVConfigs[j].pid.PID_Done;
            if (find(PID_Done.begin(), PID_Done.end(), false) != PID_Done.end())
              process_running = true;
          }
          if (process_running)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Processes of orbifold \"" << this->Orbifolds[i].OrbifoldGroup.Label << "\" are still running. Cannot be deleted." << this->Print.cend;
          }
          else
          {
            ++counter;
            this->Orbifolds.erase(this->Orbifolds.begin() + i);
            this->AllVEVConfigs.erase(this->AllVEVConfigs.begin() + i);
            this->AllVEVConfigsIndex.erase(this->AllVEVConfigsIndex.begin() + i);
          }
        }
        
        this->OrbifoldIndex = -1;
        
        if (this->print_output)
        {
          if (t1 == counter)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "All orbifolds deleted." << this->Print.cend << "\n" << endl;
          else
          {
            if (counter == 0)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "No orbifold could be deleted." << this->Print.cend << "\n" << endl;
            else
            if (counter == 1)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Only one orbifold could be deleted." << this->Print.cend << "\n" << endl;
            else
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Only " << counter << " orbifolds could be deleted." << this->Print.cend << "\n" << endl;
          }
        }
      }
      this->MessageParameterNotKnown(parameter_string1);
      return true;
    }
    

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // show directories  //menu A,   sept16
    if (this->FindCommandType1(command, "dir", parameter_string1) || this->FindCommandType1(command, "help", parameter_string1) || this->FindCommandType1(command, "ll", parameter_string1))
    {
      if (this->FindParameterType1(parameter_string1, "create random"))
      {
        (*this->Print.out) << "\n  create random orbifold from(OrbifoldLabel)\n";
        (*this->Print.out) << "  parameters:\n";
        (*this->Print.out) << "    \"save to(Filename)\"                     save result to model-file \"Filename\"\n";
        (*this->Print.out) << "    \"if(...)\"                               specify desired properties of the models to search for:\n";
        (*this->Print.out) << "                                                \"SM\", \"PS\", \"SU5\",\n";
        (*this->Print.out) << "                                                \"inequivalent\"\n";
        (*this->Print.out) << "    \"use(1,1,0,1,...)\"                      eight digits for two shifts and six Wilson lines:\n";
        (*this->Print.out) << "                                                \"1\" : take shift/WL from original model\n";
        (*this->Print.out) << "                                                \"0\" : create shift/WL randomly\n";
        (*this->Print.out) << "    \"#models(X)\"                            create \"X\" models with specified properties;\n";
        (*this->Print.out) << "                                              use \"X\" = \"all\" to create as many as possible\n";
        (*this->Print.out) << "    \"load when done\"                        after process finished, load new models\n";
        (*this->Print.out) << "  examples:\n";
        (*this->Print.out) << "    create random orbifold from(A) if(inequivalent SM) save to(File1.txt) #models(all)\n";
        (*this->Print.out) << "    create random orbifold from(B) if(inequivalent) save to(File2.txt) #models(100) use(1,1,0,0,0,0,0,0)\n\n" << flush;
      }
      else
      {
        const bool PrintSubDir = !this->FindParameterType1(parameter_string1, "no subdirectories");

        (*this->Print.out) << "\n  commands of this directory:\n";
        (*this->Print.out) << "    load orbifolds(Filename)\n";
        (*this->Print.out) << "    save orbifolds(Filename)\n\n";
        (*this->Print.out) << "    create orbifold(OrbifoldLabel) with point group(M,N)\n";
        (*this->Print.out) << "    create random orbifold from(OrbifoldLabel)\n";
        (*this->Print.out) << "                                              various parameters, see \"help create random\"\n";
        (*this->Print.out) << "    rename orbifold(OldOrbifoldLabel) to(NewOrbifoldLabel)\n";
        (*this->Print.out) << "    delete orbifold(OrbifoldLabel)\n";
        (*this->Print.out) << "    delete orbifolds\n\n";

        if (PrintSubDir)
        {
          t1 = this->Orbifolds.size();
          if (t1 != 0)
          {
            (*this->Print.out) << "  change directory:\n";
            for (i = 0; i < t1; ++i)
              (*this->Print.out) << "    cd " << this->Orbifolds[i].OrbifoldGroup.Label << "\n";
            (*this->Print.out) << "\n";
          }
        }

        (*this->Print.out) << "  general commands:\n";
        (*this->Print.out) << "    dir                                       show commands; optional: \"no subdirectories\"\n";
        (*this->Print.out) << "    help                                      optional: \"create random\"\n";
        if (!this->online_mode)
          (*this->Print.out) << "    exit                                      exit program\n";
        (*this->Print.out) << "\n" << flush;;
      }

      this->MessageParameterNotKnown(parameter_string1);
      return true;
    }        
  } //b end
  else
  {  // c begin
    // STEP 4 //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin: commands available in all orbifold directories
    COrbifold          &Orbifold       = this->Orbifolds[this->OrbifoldIndex];
    vector<SConfig>    &VEVConfigs     = this->AllVEVConfigs[this->OrbifoldIndex];
    unsigned           &VEVConfigIndex = this->AllVEVConfigsIndex[this->OrbifoldIndex];
    SConfig            &VEVConfig      = VEVConfigs[VEVConfigIndex];
    vector<CField>     &Fields         = VEVConfig.Fields;


///// begin here sept17

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // change directory
    // updated on 29.06.2011
    if (this->FindCommandType1(command, "cd ~", parameter_string1))
    {
      this->current_folder[0] = -1;
      this->current_folder[1] = 0;
      this->current_folder[2] = 0;

      this->MessageParameterNotKnown(parameter_string1);
      return true;
    }

    if (this->FindCommandType0(command, "m"))
    {
      if (Orbifold.GetCheckStatus() != CheckedAndGood)
      {
        if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
          this->MessageHelpCreateNewOrbifold(3);
        else
          this->MessageHelpCreateNewOrbifold();

        this->current_folder[1] = 1;
        return true;
      }
      this->current_folder[1] = 1;
      return true;
    }

    if (this->FindCommandType0(command, "gg"))
    {
      if (Orbifold.GetCheckStatus() != CheckedAndGood)
      {
        if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
          this->MessageHelpCreateNewOrbifold(3);
        else
          this->MessageHelpCreateNewOrbifold();

        this->current_folder[1] = 1;
        return true;
      }
      this->current_folder[1] = 2;
      return true;
    }

    if (this->FindCommandType0(command, "s"))
    {
      if (Orbifold.GetCheckStatus() != CheckedAndGood)
      {
        if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
          this->MessageHelpCreateNewOrbifold(3);
        else
          this->MessageHelpCreateNewOrbifold();

        this->current_folder[1] = 1;
        return true;
      }
      this->current_folder[1] = 3;
      return true;
    }

/*    if (this->FindCommandType0(command, "c"))
    {
      if (Orbifold.GetCheckStatus() != CheckedAndGood)
      {
        if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
          this->MessageHelpCreateNewOrbifold(3);
        else
          this->MessageHelpCreateNewOrbifold();

        this->current_folder[1] = 1;
        return true;
      }
      this->current_folder[1] = 4;
      return true;
    }
*/
    if (this->FindCommandType0(command, "v"))
    {
      if (Orbifold.GetCheckStatus() != CheckedAndGood)
      {
        if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
          this->MessageHelpCreateNewOrbifold(3);
        else
          this->MessageHelpCreateNewOrbifold();

        this->current_folder[1] = 1;
        return true;
      }
      this->current_folder[1] = 5;
      this->current_folder[2] = 0;
      return true;
    }

    if (this->FindCommandType0(command, "l"))
    {
      if (Orbifold.GetCheckStatus() != CheckedAndGood)
      {
        if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
          this->MessageHelpCreateNewOrbifold(3);
        else
          this->MessageHelpCreateNewOrbifold();

        this->current_folder[1] = 1;
        return true;
      }
      this->current_folder[1] = 5;
      this->current_folder[2] = 1;
      return true;
    }

///// end here sept17

   
    // end: commands available in all orbifold directories
    // STEP 4 //////////////////////////////////////////////////////////////////////////////////////////////////////////

    const bool UsingStandardConfig = ((VEVConfig.ConfigLabel == "StandardConfig") && (VEVConfig.ConfigNumber == 1));

    // STEP 5 //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin: commands available in sub folders
    switch (this->current_folder[1])
    { // d  begin
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // ..
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 0:
      { // e  begin
        // updated on 29.06.2011
        if (this->FindCommandType1(command, "cd ", parameter_string1))
        {
          if (this->FindParameterType1(parameter_string1, ".."))
          {
            this->current_folder[0] = -1;
            return true;
          }

          if (Orbifold.GetCheckStatus() != CheckedAndGood)
          {
            if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
              this->MessageHelpCreateNewOrbifold(3);
            else
              this->MessageHelpCreateNewOrbifold();

            this->current_folder[1] = 1;
            return true;
          }

          if (this->FindParameterType1(parameter_string1, "model"))
          {
            this->current_folder[1] = 1;
            return true;
          }

          if (this->FindParameterType1(parameter_string1, "gauge group"))
          {
            this->current_folder[1] = 2;
            return true;
          }

          if (this->FindParameterType1(parameter_string1, "spectrum"))
          {
            this->current_folder[1] = 3;
            return true;
          }

/*          if (this->FindParameterType1(parameter_string1, "couplings"))
          {
            this->current_folder[1] = 4;
            return true;
          }
*/
          if (this->FindParameterType1(parameter_string1, "vev-config/labels"))
          {
            this->current_folder[1] = 5;
            this->current_folder[2] = 1;
            return true;
          }

          if (this->FindParameterType1(parameter_string1, "vev-config"))
          {
            this->current_folder[1] = 5;
            this->current_folder[2] = 0;
            return true;
          }
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 09.09.2011  //menu B
        if (this->FindCommandType1(command, "dir", parameter_string1) || this->FindCommandType1(command, "help", parameter_string1) || this->FindCommandType1(command, "ll", parameter_string1))
        { //begin if
                     
          if (this->FindParameterType1(parameter_string1, "short cuts"))
          {
            (*this->Print.out) << "\n  short cuts:\n";
            (*this->Print.out) << "    m   change directory to /model>\n";
            (*this->Print.out) << "    gg  change directory to /gauge group>\n";
            (*this->Print.out) << "    s   change directory to /spectrum>\n";
            //(*this->Print.out) << "    c   change directory to /couplings>\n";
            (*this->Print.out) << "    v   change directory to /vev-config>\n";
            (*this->Print.out) << "    l   change directory to /vev-config/labels>\n\n" << flush;
          }
          else
          {
            (*this->Print.out) << "\n  special commands of this directory:\n";
            (*this->Print.out) << "  change directory:\n";
            (*this->Print.out) << "    cd model                                  change directory to /model>\n";
            (*this->Print.out) << "    cd gauge group                            change directory to /gauge group>\n";
            (*this->Print.out) << "    cd spectrum                               change directory to /spectrum>\n";
            //(*this->Print.out) << "    cd couplings                              change directory to /couplings>\n";
            (*this->Print.out) << "    cd vev-config                             change directory to /vev-config>\n";
            (*this->Print.out) << "    cd vev-config/labels                      change directory to /labels>\n\n";
            (*this->Print.out) << "  general commands:\n";
            (*this->Print.out) << "    dir                                       show commands\n";
            (*this->Print.out) << "    help                                      optional: \"short cuts\"\n";
            (*this->Print.out) << "    cd ..                                     leave this directory\n";
            if (!this->online_mode)
              (*this->Print.out) << "    exit                                      exit program\n";
            (*this->Print.out) << "\n" << flush;;
          }          
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }  //end if
        break;
      }  // e  end
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // model
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 1:
      {   // f  begin
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 24.01.2011
        if (this->FindCommandType1(command, "cd ..", parameter_string1))
        {
          this->current_folder[1] = 0;

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

 
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print heterotic string type
        // updated on 07.09.2011
        if (this->FindCommandType1(command, "print heterotic string type", parameter_string1))
        {
          if (this->print_output)
          {
            const SelfDualLattice Lattice = Orbifold.OrbifoldGroup.GetLattice();
            if (Lattice == E8xE8)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Using the SO(16)xSO(16) heterotic string." << this->Print.cend << "\n" << endl;
           // else
           //   if (Lattice == Spin32)
           //     (*this->Print.out) << "\n  " << this->Print.cbegin << "Using the Spin(32)/Z_2 heterotic string." << this->Print.cend << "\n" << endl;
            else
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Heterotic string not defined." << this->Print.cend << "\n" << endl;
          }

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }


        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print available space groups    (used till J27)
        // updated on 16.09.2011
/*        if (this->FindCommandType1(command, "print available space groups", parameter_string1))
        {
          const unsigned M = Orbifold.OrbifoldGroup.GetOrderZM(); //N1orig
          const unsigned N = Orbifold.OrbifoldGroup.GetOrderZN();
          this->FindSpaceGroupsInDirectory(M, N, "Geometry/");

         /* const unsigned M = Orbifold.OrbifoldGroup.GetOrderZM();   //trialN0
          const unsigned N = Orbifold.OrbifoldGroup.GetOrderZN();
          const unsigned K = Orbifold.OrbifoldGroup.GetOrderZK();
          this->FindSpaceGroupsInDirectory(M, N, K, "Geometry/");*/


/*          const bool PrintFilenames = true;

          // begin: print the space groups
          (*this->Print.out) << "\n  " << this->Print.cbegin << "available ";
          if (N != 1)
            (*this->Print.out) << "Z_" << M << " x Z_" << N;
          else
            (*this->Print.out) << "Z_" << M;
          (*this->Print.out) << " space groups: ";

          const size_t s1 = this->PV.AvailableLatticesFilenames.size();

          // no space group
          if (s1 == 0)
          {
            (*this->Print.out) << "none" << this->Print.cend << "\n" << endl;
            this->MessageParameterNotKnown(parameter_string1);
            return true;
          }

          (*this->Print.out) << this->Print.cend << "\n";

          const string space = "                                             ";
          string emptyspace = "";

          int max_length1 = 15;
          int max_length2 = 18;
          for (unsigned i = 0; PrintFilenames && (i < s1); ++i)
          {
            if (this->PV.AvailableLatticesLabels[i].size() > max_length1)
              max_length1 = this->PV.AvailableLatticesLabels[i].size();

            if (this->PV.AvailableAdditionalLabels[i].size() > max_length2)
              max_length2 = this->PV.AvailableAdditionalLabels[i].size();
          }

          (*this->Print.out) << this->Print.cbegin << "     # | lattice   ";

          emptyspace = space;
          emptyspace.resize(max_length1-10);
          (*this->Print.out) << emptyspace << " | additional label ";
          emptyspace = space;
          emptyspace.resize(max_length2-18);
          (*this->Print.out) << emptyspace << "  | geometry file" << this->Print.cend << "\n";
          (*this->Print.out) << "  " << this->Print.cbegin << "  ----------------------------------------------------------------------------------------------------- " << this->Print.cend << "\n";

          for (unsigned i = 0; i < s1; ++i)
          {
            (*this->Print.out) << "    " << this->Print.cbegin << setw(2) << i+1 << " | " << this->PV.AvailableLatticesLabels[i];
            if (this->PV.AvailableLatticesLabels[i].size() < max_length1)
            {
              emptyspace = space;
              emptyspace.resize(max_length1 - this->PV.AvailableLatticesLabels[i].size());
              (*this->Print.out) << emptyspace;
            }

            (*this->Print.out) << " | " << this->PV.AvailableAdditionalLabels[i];
            if (this->PV.AvailableAdditionalLabels[i].size() < max_length2)
            {
              emptyspace = space;
              emptyspace.resize(max_length2 - this->PV.AvailableAdditionalLabels[i].size());
              (*this->Print.out) << emptyspace;
            }

            if (PrintFilenames)
              (*this->Print.out) << " | \"" << this->PV.AvailableLatticesFilenames[i] << "\"";

            (*this->Print.out) << this->Print.cend << "\n";
          }

          // if only one space group exists and none was set before, set it automatically
          if ((s1 == 1) && (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() != CheckedAndGood))
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Only one space group, hence chosen automatically." << this->Print.cend << "\n";
            this->ExecuteOrbifoldCommand("use space group(1)");
          }
          else
            (*this->Print.out) << "\n";

          (*this->Print.out) << flush;
          // end: print the space groups

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

*/

/////////////// begin mod print available space groups J28
                 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print available space groups   
        // updated on 16.09.2011
        if (this->FindCommandType1(command, "print available space groups", parameter_string1))
        {
/*          const unsigned M = Orbifold.OrbifoldGroup.GetOrderZM(); //N1orig
          const unsigned N = Orbifold.OrbifoldGroup.GetOrderZN();
          this->FindSpaceGroupsInDirectory(M, N, "Geometry/"); */

//          const unsigned M = Orbifold.OrbifoldGroup.GetOrderZM();   //trialN0
          const unsigned N = Orbifold.OrbifoldGroup.GetOrderZN();
          const unsigned K = Orbifold.OrbifoldGroup.GetOrderZK();
          this->FindSpaceGroupsInDirectory(N, K, "Geometry/");


          const bool PrintFilenames = true;

          // begin: print the space groups
          (*this->Print.out) << "\n  " << this->Print.cbegin << "available ";
          if (K != 1)
            (*this->Print.out) << "Z_" << N << " x Z_" << K;
          else
            (*this->Print.out) << "Z_" << N;
          (*this->Print.out) << " space groups: ";

          const size_t s1 = this->PV.AvailableLatticesFilenames.size();

          // no space group
          if (s1 == 0)
          {
            (*this->Print.out) << "none" << this->Print.cend << "\n" << endl;
            this->MessageParameterNotKnown(parameter_string1);
            return true;
          }

          (*this->Print.out) << this->Print.cend << "\n";

          const string space = "                                             ";
          string emptyspace = "";

          int max_length1 = 15;
          int max_length2 = 18;
          for (unsigned i = 0; PrintFilenames && (i < s1); ++i)
          {
            if (this->PV.AvailableLatticesLabels[i].size() > max_length1)
              max_length1 = this->PV.AvailableLatticesLabels[i].size();

            if (this->PV.AvailableAdditionalLabels[i].size() > max_length2)
              max_length2 = this->PV.AvailableAdditionalLabels[i].size();
          }

          (*this->Print.out) << this->Print.cbegin << "     # | lattice   ";

          emptyspace = space;
          emptyspace.resize(max_length1-10);
          (*this->Print.out) << emptyspace << " | additional label ";
          emptyspace = space;
          emptyspace.resize(max_length2-18);
          (*this->Print.out) << emptyspace << "  | geometry file" << this->Print.cend << "\n";
          (*this->Print.out) << "  " << this->Print.cbegin << "  ----------------------------------------------------------------------------------------------------- " << this->Print.cend << "\n";

          for (unsigned i = 0; i < s1; ++i)
          {
            (*this->Print.out) << "    " << this->Print.cbegin << setw(2) << i+1 << " | " << this->PV.AvailableLatticesLabels[i];
            if (this->PV.AvailableLatticesLabels[i].size() < max_length1)
            {
              emptyspace = space;
              emptyspace.resize(max_length1 - this->PV.AvailableLatticesLabels[i].size());
              (*this->Print.out) << emptyspace;
            }

            (*this->Print.out) << " | " << this->PV.AvailableAdditionalLabels[i];
            if (this->PV.AvailableAdditionalLabels[i].size() < max_length2)
            {
              emptyspace = space;
              emptyspace.resize(max_length2 - this->PV.AvailableAdditionalLabels[i].size());
              (*this->Print.out) << emptyspace;
            }

            if (PrintFilenames)
              (*this->Print.out) << " | \"" << this->PV.AvailableLatticesFilenames[i] << "\"";

            (*this->Print.out) << this->Print.cend << "\n";
          }

          // if only one space group exists and none was set before, set it automatically
          if ((s1 == 1) && (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() != CheckedAndGood))
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Only one space group, hence chosen automatically." << this->Print.cend << "\n";
            this->ExecuteOrbifoldCommand("use space group(1)");
          }
          else
            (*this->Print.out) << "\n";

          (*this->Print.out) << flush;
          // end: print the space groups

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

/////////////// end mod print available space groups J28


//begin: adding print twist, june21,2020
//orig part from N=1
        // print ...
/*        if (this->FindCommandType1(command, "print", parameter_string1))
        {
          const COrbifoldGroup &OrbifoldGroup = Orbifold.OrbifoldGroup;
          const CSpaceGroup    &SpaceGroup    = OrbifoldGroup.GetSpaceGroup();

          if (this->FindParameterType1(parameter_string1, "twists") || this->FindParameterType1(parameter_string1, "twist"))
          {
            if (SpaceGroup.IsZMxZN())
            {
              (*this->Print.out) << "\n  v" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(0), SO8);
              (*this->Print.out) << this->Print.endofset << "\n  v" << this->Print.underscore << "2 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(1), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            else
            {
              (*this->Print.out) << "\n  v" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(0), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            (*this->Print.out) << "\n" << endl;
          }
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }              
*/
//start mod part for N=0  : opcion 1 : adding if (SpaceGroup.IsZMxZNxZK())
  // print ...
/*        if (this->FindCommandType1(command, "print", parameter_string1))
        {
          const COrbifoldGroup &OrbifoldGroup = Orbifold.OrbifoldGroup;
          const CSpaceGroup    &SpaceGroup    = OrbifoldGroup.GetSpaceGroup();

          if (this->FindParameterType1(parameter_string1, "twists") || this->FindParameterType1(parameter_string1, "twist"))
          {
            if (SpaceGroup.IsZMxZN())
            {
              (*this->Print.out) << "\n  v" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(0), SO8);
              (*this->Print.out) << this->Print.endofset << "\n  v" << this->Print.underscore << "2 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(1), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            else
            {
              (*this->Print.out) << "\n  v" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(0), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            (*this->Print.out) << "\n" << endl;
          }
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }  
*/        
//
//start mod part for N=0  : opcion 2 : replace if (SpaceGroup.IsZMxZN()) for (SpaceGroup.IsZMxZNxZK()) and that the
//otherwise be understood the if (SpaceGroup.IsZMxZN()) part.
  // print ... 
//OK form for print twists N0  
/*        if (this->FindCommandType1(command, "print", parameter_string1))
        {
          const COrbifoldGroup &OrbifoldGroup = Orbifold.OrbifoldGroup;
          const CSpaceGroup    &SpaceGroup    = OrbifoldGroup.GetSpaceGroup();

          if (this->FindParameterType1(parameter_string1, "twists") || this->FindParameterType1(parameter_string1, "twist"))
          {
            if (SpaceGroup.IsZMxZNxZK())
            {
              (*this->Print.out) << "\n  v" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(1), SO8);
              (*this->Print.out) << this->Print.endofset << "\n  v" << this->Print.underscore << "2 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(2), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            else   //uderstood if (SpaceGroup.IsZMxZN())
            {
              (*this->Print.out) << "\n  v" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(1), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            (*this->Print.out) << "\n" << endl;
          }
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }                      
*/                    

//end: adding print twist, june21, 2020



// begin reduced print, Sept9, N0

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print ...
        // updated on 24.08.2011
        if (this->FindCommandType1(command, "print", parameter_string1))
        {
          const COrbifoldGroup &OrbifoldGroup = Orbifold.OrbifoldGroup;
          const CSpaceGroup    &SpaceGroup    = OrbifoldGroup.GetSpaceGroup();

        
          if (this->FindParameterType1(parameter_string1, "orbifold label"))
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold \"" << OrbifoldGroup.Label << "\"." << this->Print.cend << "\n" << endl;
          }
          else
          // updated on 10.08.2011
          if (this->FindParameterType1(parameter_string1, "point group"))
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Point group is ";
            if (SpaceGroup.IsZMxZNxZK())
              (*this->Print.out) << "Z_" << SpaceGroup.GetN() << " x Z_" << SpaceGroup.GetK();
            else
              (*this->Print.out) << "Z_" << SpaceGroup.GetN();

            if (SpaceGroup.additional_label != "")
              (*this->Print.out) << " - " << SpaceGroup.additional_label;

            (*this->Print.out) << "." << this->Print.cend << "\n" << endl;
          }
          else
          // updated on 10.08.2011
          if (this->FindParameterType1(parameter_string1, "space group"))
          {
            if (SpaceGroup.lattice_label == "")
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Space group not chosen yet. Choose it using \"use space group\". See \"help\"." << this->Print.cend << "\n" << endl;
            else
            {
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Space group based on ";
              if (SpaceGroup.IsZMxZNxZK())
                (*this->Print.out) << "Z_" << SpaceGroup.GetN() << " x Z_" << SpaceGroup.GetK();
              else
                (*this->Print.out) << "Z_" << SpaceGroup.GetN();

              if (SpaceGroup.additional_label != "")
                (*this->Print.out) << " - " << SpaceGroup.additional_label;

              (*this->Print.out) << " point group and root-lattice of " << SpaceGroup.lattice_label << "." << this->Print.cend << "\n";
              (*this->Print.out) << "  " << this->Print.cbegin << "Generators are:" << this->Print.cend << "\n" << endl;

              const vector<CSpaceGroupElement> &SG_Generators_Twist = SpaceGroup.GetSG_Generators_Twist();
              const vector<CSpaceGroupElement> &SG_Generators_Shift = SpaceGroup.GetSG_Generators_Shift();

              t1 = SG_Generators_Twist.size();
              for (i = 0; i < t1; ++i)
              {
                if (i == 0)
                  (*this->Print.out) << "  " << this->Print.set_open;
                else
                  (*this->Print.out) << this->Print.separator << "\n  ";

                this->Print.PrintSGElement(SG_Generators_Twist[i]);
              }
              t1 = SG_Generators_Shift.size();
              for (i = 0; i < t1; ++i)
              {
                (*this->Print.out) << this->Print.separator << "\n  ";
                this->Print.PrintSGElement(SG_Generators_Shift[i]);
              }
              (*this->Print.out) << this->Print.set_close << this->Print.endofset << "\n" << endl;
            }
          }
          else
          // updated on 10.08.2011
          if (this->FindParameterType1(parameter_string1, "twists") || this->FindParameterType1(parameter_string1, "twist"))
          {
            if (SpaceGroup.IsZMxZNxZK())
            {
              (*this->Print.out) << "\n  v" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(1), SO8);
              (*this->Print.out) << this->Print.endofset << "\n  v" << this->Print.underscore << "2 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(2), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            else
            {
              (*this->Print.out) << "\n  v" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(SpaceGroup.GetTwist(1), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            (*this->Print.out) << "\n" << endl;
          }
          else
          // updated on 04.07.2011
          if (this->FindParameterType1(parameter_string1, "#SUSY"))
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "N = " << Orbifold.OrbifoldGroup.GetNumberOfSupersymmetry() << " SUSY in 4d." << this->Print.cend << "\n" << endl;
          }
          else
         //
            if (this->FindParameterType1(parameter_string1, "shifts") || this->FindParameterType1(parameter_string1, "shift"))
          {
            const SelfDualLattice Lattice = OrbifoldGroup.GetLattice();

            if (SpaceGroup.IsZMxZNxZK())
            {
              (*this->Print.out) << "\n  V" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(OrbifoldGroup.GetShift(1), Lattice);
              (*this->Print.out) << this->Print.endofset << "\n  V" << this->Print.underscore << "2 = ";
              this->Print.PrintRational(OrbifoldGroup.GetShift(2), Lattice);
              (*this->Print.out) << this->Print.endofset;
            }
            else
            {
              (*this->Print.out) << "\n  V" << this->Print.underscore << "1 = ";
              this->Print.PrintRational(OrbifoldGroup.GetShift(1), Lattice);
              (*this->Print.out) << this->Print.endofset;
            }
            (*this->Print.out) << "\n" << endl;
          }
          else
          // updated on 10.08.2011
            if (this->FindParameterType1(parameter_string1, "Wilson lines") || this->FindParameterType1(parameter_string1, "WLs"))
          {
            const SelfDualLattice   Lattice          = OrbifoldGroup.GetLattice();
            const vector<unsigned> &WL_AllowedOrders = SpaceGroup.GetWL_AllowedOrders();

            (*this->Print.out) << "\n";
            this->Print.PrintIdentifiedWilsonLines(SpaceGroup.GetWL_Relations());
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Allowed orders of the Wilson lines: ";
            for (i = 0; i < LatticeDim; ++i)
              (*this->Print.out) << WL_AllowedOrders[i] << " ";
            (*this->Print.out) << this->Print.cend << "\n\n";

            this->Print.PrintRational(OrbifoldGroup.GetWilsonLines());
            (*this->Print.out) << flush;
            if (OrbifoldGroup.UseFreelyActingWL)
            {
              (*this->Print.out) << "  " << this->Print.cbegin << "freely acting Wilson line:" << this->Print.cend << "\n  ";
              this->Print.PrintRational(OrbifoldGroup.FreelyActingWilsonLine, Lattice);
              (*this->Print.out) << "\n";
            }
            (*this->Print.out) << endl;
          }

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }



// end reduced print, Sept9   N0      



        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // heterotic string type
        // updated on 10.10.2011
/*        if (this->FindCommandType2(command, "set heterotic string type(", parameter_string1, parameter_string2))
        {
          SelfDualLattice Lattice = UNSPECIFIED_LATTICE;
          if (parameter_string1 == "E8xE8")
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Using the E8xE8 heterotic string." << this->Print.cend << "\n" << endl;
            SELFDUALLATTICE = 1;
            Lattice = E8xE8;
          }
          else
          if (parameter_string1 == "Spin32")
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Using the Spin(32)/Z_2 heterotic string." << this->Print.cend << "\n" << endl;
            SELFDUALLATTICE = 2;
            Lattice = Spin32;
          }
          else
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Use either \"E8xE8\" or \"Spin32\"." << this->Print.cend << "\n" << endl;

            this->MessageParameterNotKnown(parameter_string2);
            return true;
          }

          COrbifoldGroup &OrbifoldGroup = Orbifold.OrbifoldGroup;

          // begin: clear old model
          Orbifold.Reset(false, true, true, true);

          VEVConfigs.clear();
          VEVConfigIndex = 0;

          if (this->print_output)
            (*this->Print.out) << "  " << this->Print.cbegin << "Orbifold model \"" << OrbifoldGroup.Label << "\" cleared." << this->Print.cend << "\n";
          // end: clear old model

          OrbifoldGroup.FreelyActingWilsonLine.SetLattice(Lattice);
          CWilsonLines &WilsonLines = OrbifoldGroup.AccessWilsonLines();
          WilsonLines.SetLattice(Lattice);

          OrbifoldGroup.AccessShift(0).Lattice = Lattice;
          OrbifoldGroup.AccessShift(1).Lattice = Lattice;

          this->MessageHelpCreateNewOrbifold(1);

          this->MessageParameterNotKnown(parameter_string2);
          return true;

        }
*/
//////////////////////////

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // use space group
        // updated on 16.09.2011
        if (this->FindCommandType2(command, "use space group(", parameter_string1, parameter_string2))
        {
          (*this->Print.out) << "\n";
          if (parameter_string1.find_first_not_of("0123456789") != string::npos)
          {
            if (this->print_output)
              (*this->Print.out) << "  " << this->Print.cbegin << "Space group #" << parameter_string1 << " not known." << this->Print.cend << "\n" << endl;
            return true;
          }
          const size_t a1 = this->PV.AvailableLatticesFilenames.size();
          const unsigned lattice_number = (unsigned)atoi(parameter_string1.c_str());
          if ((lattice_number == 0) || (lattice_number > a1))
          {
            if (this->print_output)
              (*this->Print.out) << "  " << this->Print.cbegin << "Space group #" << parameter_string1 << " not known." << this->Print.cend << "\n" << endl;
            return true;
          }

          if (Orbifold.OrbifoldGroup.GetSpaceGroup_CheckStatus() == CheckedAndGood)
          {
            const CSpaceGroup &OldSpaceGroup = Orbifold.OrbifoldGroup.GetSpaceGroup();

            if ((OldSpaceGroup.lattice_label == PV.AvailableLatticesLabels[lattice_number-1]) && (OldSpaceGroup.additional_label == PV.AvailableAdditionalLabels[lattice_number-1]))
            {
              if (this->print_output)
                (*this->Print.out) << "  " << this->Print.cbegin << "Space group #" << parameter_string1 << " is already in use." << this->Print.cend << "\n" << endl;
              return true;
            }
          }

          CSpaceGroup NewSpaceGroup;
          if (!NewSpaceGroup.LoadSpaceGroup(this->PV.AvailableLatticesFilenames[lattice_number-1]))
          {
            if (this->print_output)
              (*this->Print.out) << "  " << this->Print.cbegin << "New space group could not be loaded. Thus, the model is not changed." << this->Print.cend << "\n" << endl;
            return true;
          }
          NewSpaceGroup.Check();

          if (this->print_output)
            (*this->Print.out) << "  " << this->Print.cbegin << "Now using space group #" << parameter_string1 << "." << this->Print.cend << endl;

          // begin: clear old model
          Orbifold.OrbifoldGroup.AccessSpaceGroup() = NewSpaceGroup;
          Orbifold.Reset(true, true, true, true);

          VEVConfigs.clear();
          VEVConfigIndex = 0;

          if (this->print_output)
            (*this->Print.out) << "  " << this->Print.cbegin << "Orbifold model \"" << Orbifold.OrbifoldGroup.Label << "\" cleared." << this->Print.cend << "\n";
          // end: clear old model

          // begin: print
          if (this->print_output)
          {
            (*this->Print.out) << "  " << this->Print.cbegin << "T^6 from root-lattice of " << NewSpaceGroup.lattice_label << " and ";
            if (NewSpaceGroup.IsZMxZN())
            {
              //(*this->Print.out) << this->Print.cend << "\n  " << this->Print.cbegin << "twist vector" << this->Print.cend << " v1 = ";
              //this->Print.PrintRational(NewSpaceGroup.GetTwist(0), SO8);
              //(*this->Print.out) << this->Print.endofset;
              (*this->Print.out) << this->Print.cend << "\n  " << this->Print.cbegin << "twist vector" << this->Print.cend << " v1 = ";  //<< " v2 = ";  original
              this->Print.PrintRational(NewSpaceGroup.GetTwist(1), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            
            else // added J25 afternoon
//////////////////////  begin added for ZNxZMxZK   J23night
            if (NewSpaceGroup.IsZMxZNxZK())
            {
              //(*this->Print.out) << this->Print.cend << "\n  " << this->Print.cbegin << "twist vector" << this->Print.cend << " v1 = ";
              //this->Print.PrintRational(NewSpaceGroup.GetTwist(0), SO8);
              //(*this->Print.out) << this->Print.endofset;
              (*this->Print.out) << this->Print.cend << "\n  " << this->Print.cbegin << "twist vector" << this->Print.cend << " v1 = ";  // << " v2 = "; original
              this->Print.PrintRational(NewSpaceGroup.GetTwist(1), SO8);
              
              (*this->Print.out) << this->Print.endofset;
              (*this->Print.out) << this->Print.cend << "\n  " << this->Print.cbegin << "twist vector" << this->Print.cend << " v2 = ";  // << " v3 = ";  original 
              this->Print.PrintRational(NewSpaceGroup.GetTwist(2), SO8);
              
              
              (*this->Print.out) << this->Print.endofset;
            }


//////////////////////  end added for ZNxZMxZK   J23night        
            
            
            
            else
            {
              (*this->Print.out) << "twist vector" << this->Print.cend << " v = ";
              this->Print.PrintRational(NewSpaceGroup.GetTwist(0), SO8);
              (*this->Print.out) << this->Print.endofset;
            }
            (*this->Print.out) << "\n\n";
            this->Print.PrintIdentifiedWilsonLines(NewSpaceGroup.GetWL_Relations());
            (*this->Print.out) << "\n";
          }

          this->MessageHelpCreateNewOrbifold(3);
          // end: print

          this->MessageParameterNotKnown(parameter_string2);
          return true;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // set shift
        // updated on 20.09.2011
        if (this->FindCommandType1(command, "set shift", parameter_string1))
        { //s1
          COrbifoldGroup     NewOrbifoldGroup = Orbifold.OrbifoldGroup;
          const CSpaceGroup &SpaceGroup       = NewOrbifoldGroup.GetSpaceGroup();

          const SelfDualLattice Lattice = NewOrbifoldGroup.GetLattice();
          const bool            ZMxZN   = SpaceGroup.IsZMxZN(); //orig on  (must be)
          
          const bool            ZMxZNxZK   = SpaceGroup.IsZMxZNxZK(); //added J23 night for ZNxZMxZK
          

          if (SpaceGroup.GetNumberOfSectors() == 0)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Please choose a space group first, using \"use space group(i)\". See \"help\"." << this->Print.cend << "\n" << endl;
            return true;
          }

          // begin: find parameters
          const bool b1 = this->FindParameterType1(parameter_string1, "standard embedding");
          const bool b2 = this->FindParameterType1(parameter_string1, "V");

          if ((!b1 && !b2) || (b1 && b2))
          {
            if (this->print_output)
            {
              if (ZMxZN)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "One parameter needed: \"standard embedding\" or \"V(i) = <16D vector>\"." << this->Print.cend << "\n" << endl;
              else
                (*this->Print.out) << "\n  " << this->Print.cbegin << "One parameter needed: \"standard embedding\" or \"V = <16D vector>\"." << this->Print.cend << "\n" << endl;
            }

            this->MessageParameterNotKnown(parameter_string1);
            return true;
          }
          // end: find parameters

          NewOrbifoldGroup.AccessWilsonLines().SetToZero(Lattice);

          // set shift to standard embedding V = (v_1, v_2, v_3, 0, ..., 0)
/*          if (b1)
          {
            CShiftVector newShift(Lattice);

            const CTwistVector &ZM_Twist = SpaceGroup.GetTwist(0);
            newShift[0] = ZM_Twist[1];
            newShift[1] = ZM_Twist[2];
            newShift[2] = ZM_Twist[3];
            for (i = 3; i < 16; ++i)
              newShift[i] = 0;

            NewOrbifoldGroup.AccessShift(0) = newShift;

            if (ZMxZN)
            {
              const CTwistVector &ZN_Twist = SpaceGroup.GetTwist(1);
              newShift[0] = ZN_Twist[1];
              newShift[1] = ZN_Twist[2];
              newShift[2] = ZN_Twist[3];
              for (i = 3; i < 16; ++i)
                newShift[i] = 0;

              NewOrbifoldGroup.AccessShift(1) = newShift;
            }
          }*/


//////////////////////////////////////////////
//////// begin mod part June14 for V0 = Witten internally, and V1 as a Stand Embedd. from if (b1) structure

 // set shift to standard embedding V = (v_1, v_2, v_3, 0, ..., 0)
/*          if (b1)
          {
            CShiftVector newShift(Lattice);

            //const CTwistVector &ZM_Twist = SpaceGroup.GetTwist(0);
            newShift[0] = 0;
            newShift[1] = 0;
            newShift[2] = 0;
            newShift[3] = 1;
            newShift[4] = 0;
            newShift[5] = 0;
            newShift[6] = 0;
            newShift[7] = 0;
            newShift[8] = 0;
            newShift[9] = 0;
            newShift[10] = 0;
            newShift[11] = 1;
            newShift[12] = 0;
            newShift[13] = 0;
            newShift[14] = 0;
            newShift[15] = 0;
            
            //for (i = 3; i < 16; ++i)
              //newShift[i] = 0;

            NewOrbifoldGroup.AccessShift(0) = newShift;

/*            if (ZMxZN)
            {
              const CTwistVector &ZN_Twist = SpaceGroup.GetTwist(1);
              newShift[0] = ZN_Twist[1];
              newShift[1] = ZN_Twist[2];
              newShift[2] = ZN_Twist[3];
              for (i = 3; i < 16; ++i)
                newShift[i] = 0;

              NewOrbifoldGroup.AccessShift(1) = newShift;
            }*/
		 
///////////////////// begin added June23 night for ZNxZMxZK

/*           const CTwistVector &ZN_Twist = SpaceGroup.GetTwist(1);
              newShift[0] = ZN_Twist[1];
              newShift[1] = ZN_Twist[2];
              newShift[2] = ZN_Twist[3];
              for (i = 3; i < 16; ++i)
                newShift[i] = 0;

              NewOrbifoldGroup.AccessShift(1) = newShift;

    //         if (ZMxZNxZK)
      //      {
                  
              const CTwistVector &ZK_Twist = SpaceGroup.GetTwist(2);
              newShift[0] = ZK_Twist[1];
              newShift[1] = ZK_Twist[2];
              newShift[2] = ZK_Twist[3];
              for (i = 3; i < 16; ++i)
                newShift[i] = 0;

              NewOrbifoldGroup.AccessShift(2) = newShift;
        //    }

///////////////////// end added June23 night for ZNxZMxZK
          }
*/          
          
/////////////////  bMay19
         if (b1)
          {
            CShiftVector newShift(Lattice);

            //const CTwistVector &ZM_Twist = SpaceGroup.GetTwist(0);
            newShift[0] = 0;
            newShift[1] = 0;
            newShift[2] = 0;
            newShift[3] = 1;
            newShift[4] = 0;
            newShift[5] = 0;
            newShift[6] = 0;
            newShift[7] = 0;
            newShift[8] = 0;
            newShift[9] = 0;
            newShift[10] = 0;
            newShift[11] = 1;
            newShift[12] = 0;
            newShift[13] = 0;
            newShift[14] = 0;
            newShift[15] = 0;
            
            //for (i = 3; i < 16; ++i)
              //newShift[i] = 0;

            NewOrbifoldGroup.AccessShift(0) = newShift;

/*            if (ZMxZN)
            {
              const CTwistVector &ZN_Twist = SpaceGroup.GetTwist(1);
              newShift[0] = ZN_Twist[1];
              newShift[1] = ZN_Twist[2];
              newShift[2] = ZN_Twist[3];
              for (i = 3; i < 16; ++i)
                newShift[i] = 0;

              NewOrbifoldGroup.AccessShift(1) = newShift;
            }*/
		 
///////////////////// begin added June23 night for ZNxZMxZK

           const CTwistVector &ZN_Twist = SpaceGroup.GetTwist(1);
              newShift[0] = ZN_Twist[1];
              newShift[1] = ZN_Twist[2];
              newShift[2] = ZN_Twist[3];
              for (i = 3; i < 16; ++i)
                newShift[i] = 0;

              NewOrbifoldGroup.AccessShift(1) = newShift;

             if (ZMxZNxZK)
            {                 
              const CTwistVector &ZK_Twist = SpaceGroup.GetTwist(2);
              newShift[0] = ZK_Twist[1];
              newShift[1] = ZK_Twist[2];
              newShift[2] = ZK_Twist[3];
              for (i = 3; i < 16; ++i)
                newShift[i] = 0;

              NewOrbifoldGroup.AccessShift(2) = newShift;
            }

///////////////////// end added June23 night for ZNxZMxZK
          }


///////////////// nMay19







//////// end mod part June14 for V0 = Witten internally, and V1 as a Stand Embedd. from if (b1) structure
//////////////////////////////////////////////


          // set shift to non-standard embedding
/*          if (b2)
          {
            int shift_number = -1;

            if (ZMxZN)
            {
              if (this->FindParameterType2(parameter_string1, "(", tmp_string1))
              {
                if (tmp_string1 == "1")
                  shift_number = 0;
                else
                  if (tmp_string1 == "2")
                    shift_number = 1;
              }
            }
            else
              shift_number = 0;

            rationalVector RationalVector;

            string::size_type loc1 = 0;
            bool command_ok = (shift_number != -1);
            if (command_ok)
            {
              loc1 = parameter_string1.find("=", 0);
              if (loc1 == string::npos)
                command_ok = false;
              else
                command_ok = convert_string_to_vector_of_rational(parameter_string1.substr(loc1+1, string::npos), RationalVector);
            }

            if (RationalVector.size() != 16)
              command_ok = false;

            if (!command_ok)
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Shift \"" << parameter_string1.substr(loc1+1, string::npos) << "\" failed. Nothing changed." << this->Print.cend << "\n" << endl;
              return true;
            }

            NewOrbifoldGroup.AccessShift(shift_number) = RationalVector;

            if (ZMxZN && (shift_number == 0))
            {
              CShiftVector ZeroShift(Lattice);
              NewOrbifoldGroup.AccessShift(1) = ZeroShift;

              Orbifold.OrbifoldGroup = NewOrbifoldGroup;
              Orbifold.Reset(false, false, false, false);

              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Next, select the second shift V_2 using \"set shift V(2) = <16D vector>\"." << this->Print.cend << "\n" << endl;
              return true;
            }
          }
*/
///////////////// may19 begin: if (b2)
          // set shift to non-standard embedding
          if (b2)
          {

           //begin adding V_0
           
           CShiftVector newShift(Lattice);

            //const CTwistVector &ZM_Twist = SpaceGroup.GetTwist(0);
            newShift[0] = 0;
            newShift[1] = 0;
            newShift[2] = 0;
            newShift[3] = 1;
            newShift[4] = 0;
            newShift[5] = 0;
            newShift[6] = 0;
            newShift[7] = 0;
            newShift[8] = 0;
            newShift[9] = 0;
            newShift[10] = 0;
            newShift[11] = 1;
            newShift[12] = 0;
            newShift[13] = 0;
            newShift[14] = 0;
            newShift[15] = 0;
            
            //for (i = 3; i < 16; ++i)
              //newShift[i] = 0;

            NewOrbifoldGroup.AccessShift(0) = newShift;
           
           //end adding V_0


            int shift_number = -1;

            if (ZMxZNxZK)
            {
              if (this->FindParameterType2(parameter_string1, "(", tmp_string1))
              {
                if (tmp_string1 == "1")
                  shift_number = 1;
                else
                  if (tmp_string1 == "2")
                    shift_number = 2;
              }
            }
            else
              shift_number = 1;

            rationalVector RationalVector;

            string::size_type loc1 = 0;
            bool command_ok = (shift_number != -1);
            if (command_ok)
            {
              loc1 = parameter_string1.find("=", 0);
              if (loc1 == string::npos)
                command_ok = false;
              else
                command_ok = convert_string_to_vector_of_rational(parameter_string1.substr(loc1+1, string::npos), RationalVector);
            }

            if (RationalVector.size() != 16)
              command_ok = false;

            if (!command_ok)
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Shift \"" << parameter_string1.substr(loc1+1, string::npos) << "\" failed. Nothing changed." << this->Print.cend << "\n" << endl;
              return true;
            }

            NewOrbifoldGroup.AccessShift(shift_number) = RationalVector;

            if (ZMxZNxZK && (shift_number == 1))
            {
              CShiftVector ZeroShift(Lattice);
              NewOrbifoldGroup.AccessShift(2) = ZeroShift;

              Orbifold.OrbifoldGroup = NewOrbifoldGroup;
              Orbifold.Reset(false, false, false, false);

              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Next, select the second shift V_2 using \"set shift V(2) = <16D vector>\"." << this->Print.cend << "\n" << endl;
              return true;
            }
          }



///////////////// may19 end :if  (b2)


          // begin: create and check model dependent part of the orbifold group
          NewOrbifoldGroup.CreateModelDependentPart(this->Print, this->print_output);
          if (NewOrbifoldGroup.GetModularInvariance_CheckStatus() != CheckedAndGood)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Modular invariance failed. Using old shift." << this->Print.cend << "\n" << endl;

            return true;
          }
          // end: create and check model dependent part of the orbifold group

          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Modular invariance ok." << this->Print.cend << "\n";

          // begin: clear old model
          VEVConfigs.clear();
          VEVConfigIndex = 0;

          if (this->print_output)
            (*this->Print.out) << "  " << this->Print.cbegin << "Orbifold model \"" << NewOrbifoldGroup.Label << "\" cleared. Computing new spectrum ." << flush;
          // end: clear old model

          // begin: create new model
          COrbifold NewOrbifold(NewOrbifoldGroup);
          Orbifold = NewOrbifold;

          if (this->print_output)
            (*this->Print.out) << "." << flush;

          Orbifold.CheckAnomaly(Orbifold.StandardConfig, this->GaugeIndices, this->Print, false);

          if (this->print_output)
            (*this->Print.out) << ". done." << this->Print.cend << "\n" << endl;


          SConfig TestConfig = Orbifold.StandardConfig;
          TestConfig.ConfigLabel = "TestConfig";
          TestConfig.ConfigNumber = 1;

          VEVConfigs.push_back(Orbifold.StandardConfig);
          VEVConfigs.push_back(TestConfig);
          VEVConfigIndex = 1;
          // end: create new model

          return true;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // set Wilson line
        // updated on 20.09.2011
        if (this->FindCommandType2(command, "set WL W(", parameter_string1, parameter_string2))
        {
          COrbifoldGroup NewOrbifoldGroup = Orbifold.OrbifoldGroup;

          const CSpaceGroup               &SpaceGroup       = NewOrbifoldGroup.GetSpaceGroup();
          const SelfDualLattice            Lattice          = NewOrbifoldGroup.GetLattice();
          const vector<vector<unsigned> > &WL_Relations     = SpaceGroup.GetWL_Relations();
          const vector<unsigned>          &WL_AllowedOrders = SpaceGroup.GetWL_AllowedOrders();

          int WL_number = -1;

          if (parameter_string1 == "1")
            WL_number = 0;
          else
          if (parameter_string1 == "2")
            WL_number = 1;
          else
          if (parameter_string1 == "3")
            WL_number = 2;
          else
          if (parameter_string1 == "4")
            WL_number = 3;
          else
          if (parameter_string1 == "5")
            WL_number = 4;
          else
          if (parameter_string1 == "6")
            WL_number = 5;

          rationalVector RationalVector;

          string::size_type loc1 = 0;
          bool command_ok = (WL_number != -1);
          if (command_ok)
          {
            loc1 = parameter_string2.find("=", 0);
            if (loc1 == string::npos)
              command_ok = false;
            else
              command_ok = convert_string_to_vector_of_rational(parameter_string2.substr(loc1+1, string::npos), RationalVector);
          }

          if (RationalVector.size() != 16)
            command_ok = false;

          if (!command_ok)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Wilson line \"" << parameter_string2.substr(loc1+1, string::npos) << "\" failed. Nothing changed." << this->Print.cend << "\n" << endl;
            return true;
          }

          CWilsonLine newWL(Lattice);
          newWL = RationalVector;

          const unsigned WLOrder = WL_AllowedOrders[WL_number];
          // check only if the allowed order of the wilson line is known
          if ((WLOrder != 0) && ((WLOrder % newWL.GetOrder()) != 0))
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Order of the new Wilson line failed. Using old Wilson lines." << this->Print.cend << "\n" << endl;

            return true;
          }

          NewOrbifoldGroup.AccessWilsonLines().SetWilsonLine(WL_number, newWL);

          if (this->print_output)
            (*this->Print.out) << "\n";
          // begin: change all Wilson lines that are identified on the orbifold
          t1 = WL_Relations.size();
          for (i = 0; i < t1; ++i)
          {
            const vector<unsigned> &WL_Relation = WL_Relations[i];
            if (find(WL_Relation.begin(), WL_Relation.end(), WL_number) != WL_Relation.end())
            {
              if (this->print_output)
                (*this->Print.out) << "  " << this->Print.cbegin << "Identified Wilson lines: W_" << WL_number+1;

              t2 = WL_Relation.size();
              for (j = 0; j < t2; ++j)
              {
                NewOrbifoldGroup.AccessWilsonLines().SetWilsonLine(WL_Relation[j], newWL);
                if (this->print_output && (WL_Relation[j] != WL_number))
                  (*this->Print.out) << " = W_" << WL_Relation[j] + 1;
              }
              if (this->print_output)
                (*this->Print.out) << this->Print.cend << "\n";
            }
          }
          // end: change all Wilson lines that are identified on the orbifold

          NewOrbifoldGroup.AccessWilsonLines().Check(WL_Relations, WL_AllowedOrders);

          // begin: create and check model dependent part of the orbifold group
          NewOrbifoldGroup.CreateModelDependentPart(this->Print, this->print_output);
          if (NewOrbifoldGroup.GetModularInvariance_CheckStatus() != CheckedAndGood)
          {
            if (this->print_output)
              (*this->Print.out) << "  " << this->Print.cbegin << "Modular invariance failed. Using old Wilson lines." << this->Print.cend << "\n" << endl;

            return true;
          }
          // end: create and check model dependent part of the orbifold group

          if (this->print_output)
            (*this->Print.out) << "  " << this->Print.cbegin << "Modular invariance ok." << this->Print.cend << "\n";

          // begin: clear old model
          VEVConfigs.clear();
          VEVConfigIndex = 0;

          if (this->print_output)
            (*this->Print.out) << "  " << this->Print.cbegin << "Orbifold model \"" << NewOrbifoldGroup.Label << "\" cleared. Computing new spectrum ." << flush;
          // end: clear old model

          // begin: create new model
          COrbifold NewOrbifold(NewOrbifoldGroup);
          Orbifold = NewOrbifold;

          if (this->print_output)
            (*this->Print.out) << "." << flush;

          Orbifold.CheckAnomaly(Orbifold.StandardConfig, this->GaugeIndices, this->Print, false);

          if (this->print_output)
            (*this->Print.out) << ". done." << this->Print.cend << "\n" << endl;

          SConfig TestConfig = Orbifold.StandardConfig;
          TestConfig.ConfigLabel = "TestConfig";
          TestConfig.ConfigNumber = 1;

          VEVConfigs.push_back(Orbifold.StandardConfig);
          VEVConfigs.push_back(TestConfig);
          VEVConfigIndex = 1;
          // end: create new model

          return true;
        }

//////////////////////////////

 
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 23.09.2011  //menu C
        if (this->FindCommandType1(command, "dir", parameter_string1) || this->FindCommandType1(command, "help", parameter_string1) || this->FindCommandType1(command, "ll", parameter_string1))
        { //begin if
            
          if (this->FindParameterType1(parameter_string1, "print"))
          {
            (*this->Print.out) << "\n  print commands:\n";
            (*this->Print.out) << "    print orbifold label\n";
            (*this->Print.out) << "    print heterotic string type\n";
            (*this->Print.out) << "    print available space groups\n";
            (*this->Print.out) << "    print point group\n";
            (*this->Print.out) << "    print space group\n";
            (*this->Print.out) << "    print twist\n";
            (*this->Print.out) << "    print #SUSY\n";
            (*this->Print.out) << "    print shift\n";
            (*this->Print.out) << "    print Wilson lines\n\n" << flush;
          }
          else
          if (this->FindParameterType1(parameter_string1, "short cuts"))
          {
            (*this->Print.out) << "\n  short cuts:\n";
            (*this->Print.out) << "    gg  change directory to /gauge group>\n";
            (*this->Print.out) << "    s   change directory to /spectrum>\n";
            (*this->Print.out) << "    v   change directory to /vev-config>\n";
            (*this->Print.out) << "    l   change directory to /vev-config/labels>\n\n" << flush;
          } 
          else
          {       
            const CSpaceGroup &SpaceGroup = Orbifold.OrbifoldGroup.GetSpaceGroup();
//          const bool         ZMxZN      = SpaceGroup.IsZMxZN();   //orig
            const bool         ZMxZNxZK      = SpaceGroup.IsZMxZNxZK();  //added J28

            (*this->Print.out) << "\n  special commands of this directory:\n";
            (*this->Print.out) << "    print ...                                 various parameters, see \"help print\"\n\n";
            //(*this->Print.out) << "    set heterotic string type(type)           where type can be \"E8xE8\" or \"Spin32\"\n";
            (*this->Print.out) << "    use space group(i)                        ";
           

         // this->FindSpaceGroupsInDirectory(SpaceGroup.GetM(), SpaceGroup.GetN(), "Geometry/");  //orig
            this->FindSpaceGroupsInDirectory(SpaceGroup.GetN(), SpaceGroup.GetK(), "Geometry/");  //sept17
            if (this->PV.AvailableLatticesFilenames.size() == 1)
            {
              (*this->Print.out) << "with i = 1 for " << this->PV.AvailableLatticesLabels[0];
              if (this->PV.AvailableAdditionalLabels[0] != "")
                (*this->Print.out) << " - " << this->PV.AvailableAdditionalLabels[0];
              (*this->Print.out) << "\n";
            }
            else
              this->PrintFor(this->PV.AvailableLatticesFilenames.size(), "space group", "i");
            (*this->Print.out) << "\n";

  //          if (ZMxZN)
              if (ZMxZNxZK) //sept17
              (*this->Print.out) << "    set shift V(i) = <16D vector>             for i = 1,2\n";
            else
              (*this->Print.out) << "    set shift V = <16D vector>\n";

            (*this->Print.out) << "    set shift standard embedding\n";
            (*this->Print.out) << "    set WL W(i) = <16D vector>                for i = 1,..," << LatticeDim << "\n\n";

            
            (*this->Print.out) << "  general commands:\n";
            (*this->Print.out) << "    dir                                       show commands\n";
            (*this->Print.out) << "    help                                      optional: \"print\", \"short cuts\"\n";
            (*this->Print.out) << "    cd ..                                     leave this directory\n";
            if (!this->online_mode)
              (*this->Print.out) << "    exit                                      exit program\n";
            (*this->Print.out) << "\n" << flush;;
          }
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        } // end if
        break;
      }  // f end


      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // gauge group
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
//begin put gg de U1s

      //gauge group
      case 2:
      { //g
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 18.02.2011
        if (this->FindCommandType1(command, "cd ..", parameter_string1))
        {
          this->current_folder[1] = 0;

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 29.02.2012
        // begin print
        if (this->FindCommandType1(command, "print", parameter_string1))
        {
	      // commands:
          if (this->FindParameterType1(parameter_string1, "gg") || this->FindParameterType1(parameter_string1, "gauge group"))
          {
            (*this->Print.out) << "\n";
            this->Print.PrintGaugeGroup(VEVConfig, true);
          }
          else         
          if (this->FindParameterType1(parameter_string1, "beta coefficients"))
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Beta function coefficients in vev-configuration \"" << VEVConfig.ConfigLabel << VEVConfig.ConfigNumber << "\" (N = " << VEVConfig.InvariantSupercharges.size() << " SUSY):" << this->Print.cend << "\n";

            t1 = VEVConfig.SymmetryGroup.observable_sector_GGs.size();
            for (i = 0; i < t1; ++i)
            {
              t2 = VEVConfig.SymmetryGroup.observable_sector_GGs[i];

              (*this->Print.out) << "    b_{";
              this->Print.PrintGaugeGroupFactor(VEVConfig, t2);
              (*this->Print.out) << "} = " << Analyse.ComputeBetaFunctionCoefficient(this->GaugeIndices, VEVConfig, t2) << endl;
            }
            (*this->Print.out) << endl;
          }
          else 
          if (this->FindParameterType1(parameter_string1, "simple roots"))
          {
            (*this->Print.out) << "\n";
            this->Print.PrintSimpleRoots(VEVConfig.SymmetryGroup.GaugeGroup, -1);
            (*this->Print.out) << endl;
          }
          else
          if (this->FindParameterType2(parameter_string1, "simple root(", tmp_string1))
          {
            (*this->Print.out) << "\n";
            if (tmp_string1.find_first_not_of("0123456789") != string::npos)
            {
              (*this->Print.out) << "  " << this->Print.cbegin << "Simple root #" << tmp_string1 << " not known." << this->Print.cend << "\n" << endl;
              return true;
            }
            int sr_number = (unsigned)atoi(tmp_string1.c_str()) - 1;

            this->Print.PrintSimpleRoots(VEVConfig.SymmetryGroup.GaugeGroup, (unsigned)sr_number);
            (*this->Print.out) << endl;
          }
          else
          if (this->FindParameterType1(parameter_string1, "FI term"))
          {
            (*this->Print.out) << "\n  ";
            if (VEVConfig.SymmetryGroup.IsFirstU1Anomalous)
            {
              switch(this->Print.GetOutputType())
              {
                case Tstandard:
                {
                  (*this->Print.out) << "tr Q_anom = " << setw(5) << setprecision(2) << VEVConfig.SymmetryGroup.D0_FI_term << "\n" << endl;
                  break;
                }
                case Tmathematica:
                {
                  (*this->Print.out) << "TrQanom = " << setw(5) << setprecision(2) << VEVConfig.SymmetryGroup.D0_FI_term << ";\n" << endl;
                  break;
                }
                case Tlatex:
                {
                  (*this->Print.out) << "$\\text{tr} Q_\\text{anom} = " << setw(5) << setprecision(2) << VEVConfig.SymmetryGroup.D0_FI_term << "$\n" << endl;
                  break;
                }
              }
            }
            else
              (*this->Print.out) << this->Print.cbegin << "No anomalous U(1)." << this->Print.cend << "\n" << endl;
          }
          else            
          if (this->FindParameterType1(parameter_string1, "anomaly info"))
          {
            if (this->Print.GetOutputType() == Tmathematica)
            (*this->Print.out) << "(* bash info: option \"for mathematica\" not possible for this command. *)" << endl;
            Orbifold.CheckAnomaly(VEVConfig, this->GaugeIndices, this->Print, true);
          }
          else                
          if (this->FindParameterType1(parameter_string1, "B-L generator"))
          {
            const CVector Null(16);
            if ((VEVConfig.SymmetryGroup.BmLGenerator.GetSize() == 16) && (VEVConfig.SymmetryGroup.BmLGenerator != Null))
            {
              (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1)_B-L generator:" << this->Print.cend << "\n    ";
              this->Print.PrintRational(VEVConfig.SymmetryGroup.BmLGenerator, Orbifold.OrbifoldGroup.GetLattice());
              (*this->Print.out) << "\n" << endl;
            }
            else
            {
              if (this->print_output)
                (*this->Print.out) << "  " << this->Print.cbegin << "U(1)_B-L generator not defined." << this->Print.cend << "\n" << endl;
            }
          }
          else
          if (this->FindParameterType1(parameter_string1, "U1 generators"))
          {
            (*this->Print.out) << "\n";
            this->Print.PrintU1Directions(VEVConfig.SymmetryGroup, -1);
            (*this->Print.out) << endl;
          }
          else
          if (this->FindParameterType2(parameter_string1, "U1 generator(", tmp_string1))
          {
            (*this->Print.out) << "\n";
            if (tmp_string1.find_first_not_of("0123456789") != string::npos)
            {
              (*this->Print.out) << "  " << this->Print.cbegin << "U(1) generator #" << tmp_string1 << " not known." << this->Print.cend << "\n" << endl;
              return true;
            }
            int U1_number = (unsigned)atoi(tmp_string1.c_str()) - 1;

            this->Print.PrintU1Directions(VEVConfig.SymmetryGroup, (unsigned)U1_number);
            (*this->Print.out) << endl;
          }
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }
        // end print

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // set generator of some U(1) of or U(1)_B-L and compute new charges
/*        if ((case1 = this->FindCommandType2(command, "set U1(", parameter_string1, parameter_string2)))
        {
          if (UsingStandardConfig)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;
            return true;
          }

          int U1_number = -1;

          if (case1)
          {
            if (parameter_string1.find_first_not_of("0123456789") != string::npos)
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Index \"" << parameter_string1 << "\" of U(1) generator ill-defined." << this->Print.cend << "\n" << endl;
              return true;
            }

            U1_number = (unsigned)atoi(parameter_string1.c_str()) - 1;
            const size_t u1 = VEVConfig.SymmetryGroup.GaugeGroup.u1directions.size();

            if (U1_number >= u1)
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1) generator #" << U1_number+1 << " does not exist. Number of U(1)s: " << u1 << this->Print.cend << "\n";
              return true;
            }

            if ((U1_number == 0) && VEVConfig.SymmetryGroup.IsFirstU1Anomalous)
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Cannot change the anomalous U(1) generator." << this->Print.cend << "\n" << endl;
              return true;
            }
          }
          //if (case2)
          //  U1_number = 23;

          rationalVector RationalVector;

          bool command_ok = (U1_number != -1);
          if (command_ok)
          {
            string::size_type loc1 = parameter_string2.find("=", 0);
            if (loc1 == string::npos)
              command_ok = false;
            else
              command_ok = convert_string_to_vector_of_rational(parameter_string2.substr(loc1+1, string::npos), RationalVector);
          }

          if (RationalVector.size() != 16)
            command_ok = false;

          if (!command_ok)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1) generator failed. Nothing changed." << this->Print.cend << "\n" << endl;
            return true;
          }

          CVector Generator;
          Generator = RationalVector;
          if (case1)
          {
            if (Orbifold.Config_SetU1Direction(VEVConfig, Generator, (unsigned)U1_number))
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << U1_number+1 << "-th U(1) generator changed." << this->Print.cend << "\n" << endl;
            }
            else
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1) generator failed. Nothing changed." << this->Print.cend << "\n" << endl;
            }
          }
         
          return true;
        }
*/
 
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // set generator of some U(1) of or U(1)_B-L and compute new charges
        if ((case1 = this->FindCommandType2(command, "set U1(", parameter_string1, parameter_string2)) || (case2 = this->FindCommandType1(command, "set B-L", parameter_string2)))
        {
          if (UsingStandardConfig)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;
            return true;
          }

          bool NeedsToBeNonAnomalous = true;
          if (case2)
          {
            if (this->FindParameterType1(parameter_string2, "allow for anomalous B-L"))
              NeedsToBeNonAnomalous = false;
          }

          int U1_number = -1;

          if (case1)
          {
            if (parameter_string1.find_first_not_of("0123456789") != string::npos)
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Index \"" << parameter_string1 << "\" of U(1) generator ill-defined." << this->Print.cend << "\n" << endl;
              return true;
            }

            U1_number = (unsigned)atoi(parameter_string1.c_str()) - 1;
            const size_t u1 = VEVConfig.SymmetryGroup.GaugeGroup.u1directions.size();

            if (U1_number >= u1)
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1) generator #" << U1_number+1 << " does not exist. Number of U(1)s: " << u1 << this->Print.cend << "\n";
              return true;
            }

            if ((U1_number == 0) && VEVConfig.SymmetryGroup.IsFirstU1Anomalous)
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Cannot change the anomalous U(1) generator." << this->Print.cend << "\n" << endl;
              return true;
            }
          }
          if (case2)
            U1_number = 23;

          rationalVector RationalVector;

          bool command_ok = (U1_number != -1);
          if (command_ok)
          {
            string::size_type loc1 = parameter_string2.find("=", 0);
            if (loc1 == string::npos)
              command_ok = false;
            else
              command_ok = convert_string_to_vector_of_rational(parameter_string2.substr(loc1+1, string::npos), RationalVector);
          }

          if (RationalVector.size() != 16)
            command_ok = false;

          if (!command_ok)
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1) generator failed. Nothing changed." << this->Print.cend << "\n" << endl;
            return true;
          }

          CVector Generator;
          Generator = RationalVector;
          if (case1)
          {
            if (Orbifold.Config_SetU1Direction(VEVConfig, Generator, (unsigned)U1_number))
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << U1_number+1 << "-th U(1) generator changed." << this->Print.cend << "\n" << endl;
            }
            else
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1) generator failed. Nothing changed." << this->Print.cend << "\n" << endl;
            }
          }
          if (case2)
          {
            if (Analyse.SetBmLGenerator(Orbifold, VEVConfig, Generator, NeedsToBeNonAnomalous))
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1)_B-L generator changed." << this->Print.cend << "\n" << endl;
            }
            else
            {
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "U(1)_B-L generator failed. Nothing changed." << this->Print.cend << "\n" << endl;
            }
          }
          return true;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 29.02.2012
        if (this->FindCommandType1(command, "dir", parameter_string1) || this->FindCommandType1(command, "help", parameter_string1) || this->FindCommandType1(command, "ll", parameter_string1))
        {
          const size_t s1 = VEVConfig.SymmetryGroup.GaugeGroup.u1directions.size();
          size_t s2 = 0;
          for (unsigned i = 0; i < VEVConfig.SymmetryGroup.GaugeGroup.factor.size(); ++i)
            s2 += VEVConfig.SymmetryGroup.GaugeGroup.factor[i].simpleroots.size();

         
          if (this->FindParameterType1(parameter_string1, "print"))
          {
            (*this->Print.out) << "\n  print commands:\n";
            (*this->Print.out) << "    print gauge group\n";
            (*this->Print.out) << "    print beta coefficients\n";
            if (s2 != 0)
            {
              (*this->Print.out) << "    print simple root(i)                      ";
              this->PrintFor(s2, "simple roots", "i");
              (*this->Print.out) << "    print simple roots\n";
            }
            if (s1 != 0)
            {
              (*this->Print.out) << "    print FI term\n";
              (*this->Print.out) << "    print anomaly info\n";
            }
            if (s1 != 0)
            {
              (*this->Print.out) << "    print B-L generator\n";
              (*this->Print.out) << "    print U1 generator(i)                     ";
              this->PrintFor(s1, "U(1)s", "i");
              (*this->Print.out) << "    print U1 generators\n";
            }
            (*this->Print.out) << "\n" << flush;
          }
          else
          if (this->FindParameterType1(parameter_string1, "short cuts"))
          {
            (*this->Print.out) << "\n  short cuts:\n";
            (*this->Print.out) << "    m   change directory to /model>\n";
            (*this->Print.out) << "    s   change directory to /spectrum>\n";
            (*this->Print.out) << "    v   change directory to /vev-config>\n";
            (*this->Print.out) << "    l   change directory to /vev-config/labels>\n\n" << flush;
          }
          else
          {
            (*this->Print.out) << "\n  special commands of this directory:\n";
            (*this->Print.out) << "    print ...                                 various parameters, see \"help print\"\n\n";

            if (!UsingStandardConfig)
            {
              if (s1 != 0)
              {
                (*this->Print.out) << "    set U1(i) = <16D vector>                  ";
                this->PrintFor(s1, "U(1)s", "i");
                (*this->Print.out) << "\n";
                (*this->Print.out) << "    set B-L = <16D vector>                    optional: \"allow for anomalous B-L\"\n\n";
              }
            }
            (*this->Print.out) << "  general commands:\n";
            (*this->Print.out) << "    dir                                       show commands\n";
            (*this->Print.out) << "    help                                      optional: \"print\",\"short cuts\"\n";
            (*this->Print.out) << "    cd ..                                     leave this directory\n";
            if (!this->online_mode)
              (*this->Print.out) << "    exit                                      exit program\n";
            (*this->Print.out) << "\n" << flush;;
          }
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }
        break;
      } //g
      
//end put gg de U1s

      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // spectrum
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 3:
      {  // h begin
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 18.02.2011
        if (this->FindCommandType1(command, "cd ..", parameter_string1))
        {
          this->current_folder[1] = 0;

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

 
// Begin print, June17

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print details for some fields
        // updated on 27.09.2011
/*        if (this->FindCommandType2(command, "print(", parameter_string1, parameter_string2))
        {
          SUSYMultiplet Multiplet;
          this->FindSUSYType(parameter_string2, Orbifold.OrbifoldGroup.GetNumberOfSupersymmetry(), Multiplet);

          vector<string> FieldLabels;
          ExtractLabels(Multiplet, parameter_string1, FieldLabels);
          vector<unsigned> FieldIndices = GetIndices(FieldLabels);


          const bool PrintInternalInformation = this->FindParameterType1(parameter_string2, "with internal information");

          this->Print.PrintStates(Orbifold, VEVConfig, FieldIndices, PrintInternalInformation);

          this->MessageParameterNotKnown(parameter_string2);
          return true;
        }
*/
//N=0 check
         /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print details for some fields
        // updated on 27.09.2011
        if (this->FindCommandType2(command, "print(", parameter_string1, parameter_string2))
        {
//          SUSYMultiplet Multiplet;
//          this->FindSUSYType(parameter_string2, Orbifold.OrbifoldGroup.GetNumberOfSupersymmetry(), Multiplet);

          vector<SUSYMultiplet> Multiplets(2); //Particle types to be printed    //added June17
          Multiplets[0]=Scalar;				//and to be given for equivalence check   //added June17
	      Multiplets[1]=LeftFermi;      //added June17
	      
          vector<string> FieldLabels;
          ExtractLabels(Multiplets, parameter_string1, FieldLabels);   //Multiplet a Multiplets, June17 
          vector<unsigned> FieldIndices = GetIndices(FieldLabels);

          // find conditions and apply them
          if (!this->FindConditionsAndFilterFieldIndices(parameter_string2, FieldIndices))
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "Conditions failed." << this->Print.cend << "\n" << endl;
            this->MessageParameterNotKnown(parameter_string2);
            return true;
          }
          if (FieldIndices.size() == 0)
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "No state to print." << this->Print.cend << "\n" << endl;
            this->MessageParameterNotKnown(parameter_string2);
            return true;
          }

          const bool PrintInternalInformation = this->FindParameterType1(parameter_string2, "with internal information");

          this->Print.PrintStates(Orbifold, VEVConfig, FieldIndices, PrintInternalInformation);
 
          this->MessageParameterNotKnown(parameter_string2);
          return true;
        }
 
//// End print, June17
 
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print summary ...
        // updated on 15.11.2011
        /*if (this->FindCommandType1(command, "local spectrum(", parameter_string1))
        {
        bool FixedBraneFound = false;
        const CFixedBrane &FixedBrane = this->AccessFixedBrane(parameter_string1, Orbifold, FixedBraneFound);
          
        tmp_string1 = "local";
        CFixedBrane LocalBrane(FixedBrane.GetSGElement(), 0, tmp_string1);

          //LocalBrane.SortByEigenvalue(OrbifoldGroup, Sector.GetLM_all_Oscillators(), Gamma_Centralizer);
          //LocalBrane.CreateStates(Sector.GetRightMovers(), OrbifoldGroup.GetCentralizer(FixedBrane.Index_SGElement), Gamma_Centralizer);

      }*/
        
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print summary ...
        // updated on 12.09.2011
/*        if (this->FindCommandType1(command, "print summary", parameter_string1))
        {
         
          SUSYMultiplet Multiplet;
          this->FindSUSYType(parameter_string1, Orbifold.OrbifoldGroup.GetNumberOfSupersymmetry(), Multiplet);


            (*this->Print.out) << "\n";
           // this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplet, print_labels);
             this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplet, false);
            (*this->Print.out) << "\n";
         

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }
 
*/

// from N0U1s
// el que estaba antes de sept20
 // print summary ...
/*h        if (this->FindCommandType1(command, "print summary", parameter_string1))
        {
			
		  const bool print_labels      = this->FindParameterType1(parameter_string1, "with labels"); //added June17	
			
			
//          SUSYMultiplet Multiplet;
//          this->FindSUSYType(parameter_string1, Orbifold.OrbifoldGroup.GetNumberOfSupersymmetry(), Multiplet);
          
         // (*this->Print.out) << "\n ";   //comment in original practprompt-May20 
         // this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplet, false);  //comment in original practprompt-May20 
         // (*this->Print.out) << "\n ";  //comment in original practprompt-May20 
          
          
          vector<SUSYMultiplet> Multiplets(2);						//Particle types to be printed
	      Multiplets[0]=Scalar;									//and to be given for equivalence check
	      Multiplets[1]=LeftFermi;
          
   //       Printtxt.PrintSummaryOfVEVConfig(AllVEVConfigs[j], Multiplets, false);    //comment in original practprompt-May20 
          //Print.PrintSummaryOfVEVConfig(Orbifold.StandardConfig, Multiplets, false); //comment in original practprompt-May20 
          //this->Print.PrintSummaryOfVEVConfig(Orbifold.StandardConfig, Multiplets, false); //comment in original practprompt-May20 
          
 //         this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplets, false);  //original practprompt-May20
          
          (*this->Print.out) << "\n";    //added June17
          this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplets, print_labels);  //added June17
          (*this->Print.out) << "\n";   //added June17

          
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }  
*/ //h


///////// begin try all print , sept20

                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print summary ...
        // updated on 12.09.2011
        if (this->FindCommandType1(command, "print summary", parameter_string1))
        {
          const bool print_NoU1Charges = this->FindParameterType1(parameter_string1, "no U1s");
          const bool print_labels      = this->FindParameterType1(parameter_string1, "with labels");

//          SUSYMultiplet Multiplet;
//          this->FindSUSYType(parameter_string1, Orbifold.OrbifoldGroup.GetNumberOfSupersymmetry(), Multiplet);

          vector<SUSYMultiplet> Multiplets(2);						//Particle types to be printed
	      Multiplets[0]=Scalar;									//and to be given for equivalence check
	      Multiplets[1]=LeftFermi;

          const vector<unsigned> Orig_observable_sector_U1s = VEVConfig.SymmetryGroup.observable_sector_U1s;
          if (print_NoU1Charges)
            VEVConfig.SymmetryGroup.observable_sector_U1s.clear();

         if (this->FindParameterType1(parameter_string1, " of sectors"))         //sept20
          {
            (*this->Print.out) << "\n";
            this->Print.PrintSummaryOfSectors(Orbifold, VEVConfig, Multiplets, print_labels);
          }
          else
          if (this->FindParameterType1(parameter_string1, " of fixed points"))   //sept21
          {
            (*this->Print.out) << "\n";
            this->Print.PrintSummaryOfFixedBranes(Orbifold, VEVConfig, Multiplets, print_labels);
          }
          else
          // print summary of twisted sector T(k,l) (use T(0,0) for untwisted sector)
          if (this->FindParameterType2(parameter_string1, "of sector T(", tmp_string1))
          {
            (*this->Print.out) << "\n";

            vector<int> unsigned_Sector;
            convert_string_to_vector_of_int(tmp_string1, unsigned_Sector);

//            const SUSYMultiplet &Multiplet = Scalar; //added

//            if (unsigned_Sector.size() == 2)
            if (unsigned_Sector.size() == 3)
            {
              t1 = Orbifold.GetNumberOfSectors();
              for (i = 0; i < t1; ++i)
              {
                const CSector &current_Sector = Orbifold.GetSector(i);
//                if ((current_Sector.Get_k() == unsigned_Sector[0]) && (current_Sector.Get_l() == unsigned_Sector[1]))
                if ((current_Sector.Get_m() == unsigned_Sector[0]) && (current_Sector.Get_n() == unsigned_Sector[1]) && (current_Sector.Get_k() == unsigned_Sector[2]))
                {
//                  this->Print.PrintSummary(current_Sector, VEVConfig, Multiplet, print_labels);
                  for (int j=0; j<2; j++) //added july 4,2023
                  {  //added
                  this->Print.PrintSummary(current_Sector, VEVConfig, Multiplets[j], print_labels);
                  (*this->Print.out) << endl;
			      } //added
                  return true;
                }
              }
            }
            (*this->Print.out) << "  " << this->Print.cbegin << "Sector \"" << tmp_string1 << "\" not known." << this->Print.cend << "\n" << endl;
          }
          else
//////////////////////
          // print summary of fixed point
          if (this->FindParameterType2(parameter_string1, "of fixed point(", tmp_string1))
          {
            bool FixedBraneFound = false;
            const CFixedBrane &FixedBrane = this->AccessFixedBrane(tmp_string1, Orbifold, FixedBraneFound);

            (*this->Print.out) << "\n";
            if (FixedBraneFound)
              for (int j=0; j<2; j++) //added july 4, 2023
              {  //added
              this->Print.PrintSummary(FixedBrane, Orbifold.OrbifoldGroup, VEVConfig, Multiplets[j], print_labels);
		      } // added
            else
              (*this->Print.out) << "  " << this->Print.cbegin << "Fixed point \"" << tmp_string1 << "\" not known." << this->Print.cend << "\n";
            (*this->Print.out) << endl;
		      
          }
          else 
//////////////////////                              
          {
            (*this->Print.out) << "\n";
            this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplets, print_labels);
            (*this->Print.out) << "\n";
          }
          if (print_NoU1Charges)
            VEVConfig.SymmetryGroup.observable_sector_U1s = Orig_observable_sector_U1s;
            
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

///////// end try all print , sept20

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print all states   //sept23
        // updated on 11.05.2011
        if (this->FindCommandType1(command, "print all states", parameter_string1)) //sept23
        {
          this->Print.PrintStates(Orbifold, VEVConfig);

          this->MessageParameterNotKnown(parameter_string1);
          return true;
        }

////////////// July 5, 2023, begin tex table try1
       
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // tex details for some fields
        // updated on 21.09.2011
        if (this->FindCommandType2(command, "tex table(", parameter_string1, parameter_string2))
        {
//          SUSYMultiplet Multiplet;
//          this->FindSUSYType(parameter_string2, Orbifold.OrbifoldGroup.GetNumberOfSupersymmetry(), Multiplet);
          vector<SUSYMultiplet> Multiplets(2); //sept23    
          Multiplets[0]=Scalar;				
	      Multiplets[1]=LeftFermi;      

          vector<string> FieldLabels;
//          ExtractLabels(Multiplet, parameter_string1, FieldLabels);
          ExtractLabels(Multiplets, parameter_string1, FieldLabels);
          vector<unsigned> FieldIndices = GetIndices(FieldLabels);

          // find conditions and apply them
          if (!this->FindConditionsAndFilterFieldIndices(parameter_string2, FieldIndices))
          {
            cout << "Warning in bool CPrompt::ExecuteCommand(...): Could not apply the conditions. Return false." << endl;
            return false;
          }

          // begin: choose the details that will be listed
          this->FindParameterType2(parameter_string2, "print labels(", tmp_string1);

          vector<int> int_PrintLabels;
          convert_string_to_vector_of_int(tmp_string1, int_PrintLabels);

          unsigned number_of_Labels = 0;
          const size_t f1 = VEVConfig.Fields.size();
          if (f1 != 0)
            number_of_Labels = VEVConfig.Fields[0].Labels.size();

          t1 = int_PrintLabels.size();
          vector<unsigned> PrintLabels;
          for (i = 0; i < t1; ++i)
          {
            if ((int_PrintLabels[i] > 0) && (int_PrintLabels[i] <= number_of_Labels))
              PrintLabels.push_back((unsigned)(int_PrintLabels[i]-1));
            else
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Label #" << int_PrintLabels[i] << " not known." << this->Print.cend << endl;
          }
          // end: choose the details that will be listed

          if (FieldIndices.size() == 0)
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "No state to print." << this->Print.cend << "\n" << endl;
            return true;
          }

          (*this->Print.out) << "\\documentclass[a4paper,12pt,twoside]{article}\n";
          (*this->Print.out) << "\\usepackage{longtable}\n";
          (*this->Print.out) << "\\usepackage{amsmath}\n\n";
          (*this->Print.out) << "\\newcommand{\\rep}[1]{\\ensuremath\\boldsymbol{#1}}\n";
          (*this->Print.out) << "\\newcommand{\\crep}[1]{\\ensuremath\\overline{\\boldsymbol{#1}}}\n\n";
          (*this->Print.out) << "\\begin{document}\n";
          (*this->Print.out) << "\\addtolength{\\hoffset}{-1.5cm}\n";

          this->Print.TexSpectrum(Orbifold, VEVConfig, FieldIndices, PrintLabels);
          (*this->Print.out) << "\n\\end{document}\n";

          this->MessageParameterNotKnown(parameter_string2);
          return true;
        }

 
        

////////////// July 5, 2023, end tex table try1



         /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // print list of charges  //sept23
        // updated on 21.06.2011
        if (this->FindCommandType2(command, "print list of charges(", parameter_string1, parameter_string2))
        {
          
          vector<SUSYMultiplet> Multiplets(2); //sept23    
          Multiplets[0]=Scalar;				
	      Multiplets[1]=LeftFermi;      
          
          vector<string> FieldLabels;
          ExtractLabels(Multiplets, parameter_string1, FieldLabels);  // Multiplet a Multiplets, //sept23
          vector<unsigned> FieldIndices = GetIndices(FieldLabels);

          // find conditions and apply them
          if (!this->FindConditionsAndFilterFieldIndices(parameter_string2, FieldIndices))
          {
            cout << "Warning in bool CPrompt::ExecuteCommand(...): Could not apply the conditions. Return false." << endl;
            return false;
          }

          tmp_string1 = "";
          this->FindParameterType2(parameter_string2, "label of list(", tmp_string1);

          if (FieldIndices.size() == 0)
          {
            (*this->Print.out) << "\n  " << this->Print.cbegin << "No state to print." << this->Print.cend << "\n" << endl;

            this->MessageParameterNotKnown(parameter_string2);
            return true;
          }
          this->Print.PrintListOfCharges(Orbifold, VEVConfig, FieldIndices, tmp_string1);
          (*this->Print.out) << endl;

          this->MessageParameterNotKnown(parameter_string2);
          return true;
        }       
        
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 16.09.2011
/*        if (this->FindCommandType1(command, "dir", parameter_string1) || this->FindCommandType1(command, "help", parameter_string1) || this->FindCommandType1(command, "ll", parameter_string1))
        { //begin if
         
           
          
            (*this->Print.out) << "\n  special commands of this directory:\n";
          
            (*this->Print.out) << "    print summary \n";
           
            
            (*this->Print.out) << "  general commands:\n";
            (*this->Print.out) << "    dir (or help)                             show commands\n";
     
            (*this->Print.out) << "    cd ..                                     leave this directory\n";
            if (!this->online_mode)
              (*this->Print.out) << "    exit                                      exit program\n";
            (*this->Print.out) << "\n" << flush;;
          
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        } //end if
        break;
      }  // h  end
*/      
      
 //////// b new     
      
         /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // updated on 16.09.2011
        if (this->FindCommandType1(command, "dir", parameter_string1) || this->FindCommandType1(command, "help", parameter_string1) || this->FindCommandType1(command, "ll", parameter_string1))
        { //begin if
          if (this->FindParameterType1(parameter_string1, "print summary"))
          {
            (*this->Print.out) << "\n  print summary\n";
            (*this->Print.out) << "    print a summary table of massless fields\n\n";
            (*this->Print.out) << "  parameters:\n";
            (*this->Print.out) << "    \"of sectors\"                            group table by sectors\n";
            (*this->Print.out) << "    \"of fixed points\"                       group table by fixed points\n";
            (*this->Print.out) << "    \"no U1s\"                                omit the U(1) charges\n";
            (*this->Print.out) << "    \"with labels\"                           print the field labels\n\n" << flush;
          }
          else
          if (this->FindParameterType1(parameter_string1, "short cuts"))
          {
            (*this->Print.out) << "\n  short cuts:\n";
            (*this->Print.out) << "    m   change directory to /model>\n";
            (*this->Print.out) << "    gg  change directory to /gauge group>\n";
            (*this->Print.out) << "    v   change directory to /vev-config>\n";
            (*this->Print.out) << "    l   change directory to /vev-config/labels>\n\n" << flush;
          }
          else
          {
            (*this->Print.out) << "\n  special commands of this directory:\n";
            (*this->Print.out) << "    print(fields)                             optional: \"with internal information\"\n";
            (*this->Print.out) << "    print all states\n";
            (*this->Print.out) << "    print summary                             various parameters, see \"help print summary\"\n";
            (*this->Print.out) << "    print list of charges(fields)\n";
            (*this->Print.out) << "  general commands:\n";
            (*this->Print.out) << "    dir (or help)                             show commands\n";
            (*this->Print.out) << "                                              optional: \"short cuts\", \"print summary\"\n";                                                       
            (*this->Print.out) << "    cd ..                                     leave this directory\n";
            if (!this->online_mode)
              (*this->Print.out) << "    exit                                      exit program\n";
            (*this->Print.out) << "\n" << flush;;
          }
          this->MessageParameterNotKnown(parameter_string1);
          return true;
        } //end if
        break;
      } //h end
        
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // couplings
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /*   case 4:
      { // i  begin
        
      } // i  end */
 
 
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // vev-config
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
//begin put case 5 vev-config from U1, use index j here

      case 5:
      { //j
        switch (this->current_folder[2])
        { //k
          ////////////////////////////////////////////////////////////////////////////////////////////////////////////
          // ..
          ////////////////////////////////////////////////////////////////////////////////////////////////////////////
          case 0:
          {  //l
            bool print_configs = false;
            bool print_help    = false;

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 04.09.2011
            if (this->FindCommandType1(command, "dir", parameter_string1) || this->FindCommandType1(command, "help", parameter_string1) || this->FindCommandType1(command, "ll", parameter_string1))
            {
              if (parameter_string1 == "")
                print_configs = true;

              print_help  = true;
            }

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 18.02.2011
            if (this->FindCommandType1(command, "cd labels", parameter_string1))
            {
              this->current_folder[2] = 1;

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            }

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 18.02.2011
            if (this->FindCommandType1(command, "cd ..", parameter_string1))
            {
              this->current_folder[1] = 0;

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            }

//sept24
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 04.09.2011
            if (print_configs || this->FindCommandType1(command, "print configurations", parameter_string1) || this->FindCommandType1(command, "print configs", parameter_string1))
            {
              int max_length = 0;
              int length     = 0;
              
              t1 = VEVConfigs.size();
              if (t1 > 0)
              {
                std::ostringstream os;
                os << VEVConfigs[0].ConfigNumber;
                tmp_string1 = os.str();
                max_length = VEVConfigs[0].ConfigLabel.size() + tmp_string1.size();
              }
              
              for (i = 1; i < t1; ++i)
              {
                std::ostringstream os;
                os << VEVConfigs[i].ConfigNumber;
                tmp_string1 = os.str();
                length = VEVConfigs[i].ConfigLabel.size() + tmp_string1.size();
                
                if (length > max_length)
                  max_length = length;
              }

              (*this->Print.out) << "\n  " << this->Print.cbegin << "list of vev-configurations: " << this->Print.cend << "\n";
              (*this->Print.out) << "  " << this->Print.cbegin << "   label ";
              for (i = 3; i < max_length; ++i)
                (*this->Print.out) << " ";
              (*this->Print.out) << "| field label # | fields with VEV " << this->Print.cend << "\n";
              (*this->Print.out) << "  " << this->Print.cbegin << "  ----------------------------------------------------------------------------------------------------- " << this->Print.cend << "\n";

              unsigned counter_printed_VEVs = 0;
              for (i = 0; i < t1; ++i)
              {
                const SConfig &VEVConfig = VEVConfigs[i];
                if (i == VEVConfigIndex)
                  (*this->Print.out) << "  -> ";
                else
                  (*this->Print.out) << "     ";

                (*this->Print.out) << "\"" << VEVConfig.ConfigLabel << VEVConfig.ConfigNumber << "\"";

                std::ostringstream os;
                os << VEVConfig.ConfigNumber;
                tmp_string1 = os.str();
                length = VEVConfig.ConfigLabel.size() + tmp_string1.size();
                for (j = length; j < max_length; ++j)
                  (*this->Print.out) << " ";
                
                const size_t f1 = VEVConfig.Fields.size();
                if (f1 == 0)
                {
                  cout << "Warning in bool CPrompt::ExecuteCommand(...): \"Fields\" is empty. Return true." << endl;
                  return true;  
                }
                const unsigned number_of_labels = VEVConfig.Fields[0].Labels.size();
                (*this->Print.out) << " |      " << setw(3) << VEVConfig.use_Labels + 1 << " /" << setw(3) << number_of_labels << " | ";
                counter_printed_VEVs = 0;
                for (j = 0; j < f1; ++j)
                {
                  const CField &Field = VEVConfig.Fields[j];
                  if (!Field.VEVs.IsZero())
                  {
                    (*this->Print.out) << "<";
                    this->Print.PrintLabel(Field, VEVConfig.use_Labels);
                    (*this->Print.out) << "> ";
                    ++counter_printed_VEVs;
                    if (counter_printed_VEVs % 10 == 0)
                    {
                      (*this->Print.out) << "\n       ";
                      for (k = 0; k < max_length; ++k)
                        (*this->Print.out) << " ";
                      (*this->Print.out) << " |               | ";
                    }
                  }
                }
                (*this->Print.out) << "\n";
              }
              if (print_help)
                (*this->Print.out) << flush;
              else
              {
                (*this->Print.out) << endl;
                this->MessageParameterNotKnown(parameter_string1);
                return true;
              }
            }

                      
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 26.08.2011
            if (this->FindCommandType2(command, "use config(", parameter_string1, parameter_string2))
            {
              string   useVEVConfigLabel  = parameter_string1;
              unsigned useVEVConfigNumber = 1;
              this->SplitVEVConfigLabel(useVEVConfigLabel, useVEVConfigNumber);

              unsigned index = 0;
              if (this->MessageVEVConfigNotKnown(VEVConfigs, useVEVConfigLabel, useVEVConfigNumber, index))
              {
                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }
              if (VEVConfigIndex == index)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"" << useVEVConfigLabel << useVEVConfigNumber << "\" is already in use." << this->Print.cend << "\n" << endl;
              }
              else
              {
                VEVConfigIndex = index;

                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Now using vev-configuration \"" << useVEVConfigLabel << useVEVConfigNumber << "\"." << this->Print.cend << "\n" << endl;
              }

              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }

//sept24    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 15.07.2011
            if (this->FindCommandType2(command, "create config(", parameter_string1, parameter_string2))
            {
              // begin: find parameters
              string   fromVEVConfigLabel  = "StandardConfig";
              unsigned fromVEVConfigNumber = 1;

              // if no other origin is specified use the standard config to create the new config
              if (this->FindParameterType2(parameter_string2, "from(", fromVEVConfigLabel))
                this->SplitVEVConfigLabel(fromVEVConfigLabel, fromVEVConfigNumber);
              // end: find parameters

              // begin: new config label
              if (this->MessageLabelError(parameter_string1))
              {
                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }

              string   newVEVConfigLabel  = parameter_string1;
              unsigned newVEVConfigNumber = 1;
              this->SplitVEVConfigLabel(newVEVConfigLabel, newVEVConfigNumber);

              while (this->MessageVEVConfigAlreadyExists(VEVConfigs, newVEVConfigLabel, newVEVConfigNumber))
                ++newVEVConfigNumber;
              // end: new config label

              unsigned index = 0;
              if (this->MessageVEVConfigNotKnown(VEVConfigs, fromVEVConfigLabel, fromVEVConfigNumber, index))
              {
                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }

              SConfig NewVEVConfig = VEVConfigs[index];
              NewVEVConfig.ConfigLabel  = newVEVConfigLabel;
              NewVEVConfig.ConfigNumber = newVEVConfigNumber;

              VEVConfigIndex = VEVConfigs.size();
              VEVConfigs.push_back(NewVEVConfig);

              if (this->print_output)
              {
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"" << newVEVConfigLabel << newVEVConfigNumber << "\" created from \"" << fromVEVConfigLabel << fromVEVConfigNumber << "\"." << this->Print.cend;
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Now using new configuration \"" << newVEVConfigLabel << newVEVConfigNumber << "\"." << this->Print.cend << "\n" << endl;
              }

              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }


//sept24
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 15.07.2011
            if (this->FindCommandType2(command, "rename config(", parameter_string1, parameter_string2))
            {
              // begin: find parameters
              if (!this->FindParameterType2(parameter_string2, "to(", tmp_string1))
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "New label of vev-configuration not specified." << this->Print.cend << "\n" << endl;

                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }
              // end: find parameters

              // begin: from config
              string   fromVEVConfigLabel  = parameter_string1;
              unsigned fromVEVConfigNumber = 1;
              this->SplitVEVConfigLabel(fromVEVConfigLabel, fromVEVConfigNumber);

              if ((fromVEVConfigLabel == "StandardConfig") && (fromVEVConfigNumber == 1))
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;

                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }

              unsigned index = 0;
              if (this->MessageVEVConfigNotKnown(VEVConfigs, fromVEVConfigLabel, fromVEVConfigNumber, index))
              {
                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }
              // end: from config

              // begin: to config
              if (this->MessageLabelError(tmp_string1))
              {
                this->MessageParameterNotKnown(tmp_string1);
                return true;
              }

              string   newVEVConfigLabel  = tmp_string1;
              unsigned newVEVConfigNumber = 1;
              this->SplitVEVConfigLabel(newVEVConfigLabel, newVEVConfigNumber);

              while (this->MessageVEVConfigAlreadyExists(VEVConfigs, newVEVConfigLabel, newVEVConfigNumber))
                ++newVEVConfigNumber;
              // end: to config

              SConfig &RenamedVEVConfig = VEVConfigs[index];
              RenamedVEVConfig.ConfigLabel  = newVEVConfigLabel;
              RenamedVEVConfig.ConfigNumber = newVEVConfigNumber;

              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \""  << fromVEVConfigLabel << fromVEVConfigNumber << "\" renamed to \"" << newVEVConfigLabel << newVEVConfigNumber << "\"." << this->Print.cend << "\n" << endl;

              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }

//sept24
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 24.10.2011
            if (this->FindCommandType2(command, "delete config(", parameter_string1, parameter_string2))
            {
              string   deleteVEVConfigLabel  = parameter_string1;
              unsigned deleteVEVConfigNumber = 1;
              this->SplitVEVConfigLabel(deleteVEVConfigLabel, deleteVEVConfigNumber);

              if ((deleteVEVConfigLabel == "StandardConfig") && (deleteVEVConfigNumber == 1))
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;

                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }

              unsigned index = 0;
              if (this->MessageVEVConfigNotKnown(VEVConfigs, deleteVEVConfigLabel, deleteVEVConfigNumber, index))
              {
                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }
              
              const vector<bool> &PID_Done = VEVConfigs[index].pid.PID_Done;
              if (find(PID_Done.begin(), PID_Done.end(), false) != PID_Done.end())
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Processes of vev-configuration \"" << VEVConfigs[index].ConfigLabel << VEVConfigs[index].ConfigNumber << "\" are still running. Cannot be deleted." << this->Print.cend << "\n" << endl;
              }
              else
              {
                VEVConfigs.erase(VEVConfigs.begin() + index);
                VEVConfigIndex = index-1;

                if (this->print_output)
                {
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"" << deleteVEVConfigLabel << deleteVEVConfigNumber << "\" deleted." << this->Print.cend;
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Now using vev-configuration \"" << VEVConfigs[VEVConfigIndex].ConfigLabel << VEVConfigs[VEVConfigIndex].ConfigNumber << "\"." << this->Print.cend << "\n" << endl;
                }
              }

              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }

           
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 15.07.2011
            if (this->FindCommandType1(command, "print gauge group", parameter_string1))
            {
              (*this->Print.out) << "\n";
              this->Print.PrintGaugeGroup(VEVConfig, true);

              if ((VEVConfig.SymmetryGroup.observable_sector_GGs.size() != VEVConfig.SymmetryGroup.GaugeGroup.factor.size())
               || (VEVConfig.SymmetryGroup.observable_sector_U1s.size() != VEVConfig.SymmetryGroup.GaugeGroup.u1directions.size()))
                (*this->Print.out) << "  " << this->Print.cbegin << "(factors in brackets, e.g. [SU(2)], belong to the hidden sector in this vev-configuration)" << this->Print.cend << "\n\n";

              (*this->Print.out) << flush;

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            }

///////////////////// added 7-03 select obs sect
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 27.01.2012
            if (this->FindCommandType1(command, "select observable sector:", parameter_string1))
            {
              if (UsingStandardConfig)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;
                return true;
              }

              bool ok = true;

              // select gauge group factors
              if (this->FindParameterType1(parameter_string1, "full gauge group"))
              {
                t1 = VEVConfig.SymmetryGroup.GaugeGroup.factor.size();
                VEVConfig.SymmetryGroup.observable_sector_GGs.clear();
                for (i = 0; i < t1; ++i)
                  VEVConfig.SymmetryGroup.observable_sector_GGs.push_back(i);

                if (this->print_output)
                {
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Observable sector changed." << this->Print.cend << "\n";
                  this->Print.PrintGaugeGroup(VEVConfig, true);
                }
              }
              else
              if (this->FindParameterType1(parameter_string1, "no gauge groups"))
              {
                VEVConfig.SymmetryGroup.observable_sector_GGs.clear();

                if (this->print_output)
                {
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Observable sector changed." << this->Print.cend << "\n";
                  this->Print.PrintGaugeGroup(VEVConfig, true);
                }
              }
              else
              if (this->FindParameterType2(parameter_string1, "gauge group(", parameter_string2))
              {
                t1 = VEVConfig.SymmetryGroup.GaugeGroup.factor.size();
                VEVConfig.SymmetryGroup.observable_sector_GGs.clear();

                vector<int> ggs;
                t2 = 0;

                ok = true;
                if ((parameter_string2 != "") && (parameter_string2.find_first_not_of(",0123456789") == string::npos))
                {
                  convert_string_to_vector_of_int(parameter_string2, ggs);
                  t2 = ggs.size();

                  for (i = 0; ok && (i < t2); ++i)
                  {
                    if ((ggs[i] <= 0) || (ggs[i] > t1))
                      ok = false;
                  }
                }

                if (ok)
                {
                  for (i = 0; i < t2; ++i)
                    VEVConfig.SymmetryGroup.observable_sector_GGs.push_back(ggs[i]-1);

                  if (this->print_output)
                  {
                    (*this->Print.out) << "\n  " << this->Print.cbegin << "Observable sector changed:" << this->Print.cend;
                    this->Print.PrintGaugeGroup(VEVConfig, true);
                  }
                }
                else
                {
                  if (this->print_output)
                    (*this->Print.out) << "\n  " << this->Print.cbegin << "Observable sector not valid." << this->Print.cend << endl;
                }
              }

              // select all U(1) factors
              if (this->FindParameterType1(parameter_string1, "all U1s"))
              {
                t1 = VEVConfig.SymmetryGroup.GaugeGroup.u1directions.size();
                VEVConfig.SymmetryGroup.observable_sector_U1s.clear();
                for (i = 0; i < t1; ++i)
                  VEVConfig.SymmetryGroup.observable_sector_U1s.push_back(i);

                if (this->print_output)
                {
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Observable sector changed." << this->Print.cend;
                  this->Print.PrintGaugeGroup(VEVConfig, true);
                }
              }
              else
              // select U(1) factors
              if (this->FindParameterType1(parameter_string1, " no U1s"))
              {
                VEVConfig.SymmetryGroup.observable_sector_U1s.clear();

                if (this->print_output)
                {
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Observable sector changed." << this->Print.cend;
                  this->Print.PrintGaugeGroup(VEVConfig, true);
                }
              }
              else
              if (this->FindParameterType2(parameter_string1, " U1s(", parameter_string2))
              {
                t1 = VEVConfig.SymmetryGroup.GaugeGroup.u1directions.size();
                VEVConfig.SymmetryGroup.observable_sector_U1s.clear();

                vector<int> u1s;
                t2 = 0;
                
                ok = true;
                if ((parameter_string2 != "") && (parameter_string2.find_first_not_of(",0123456789") == string::npos))
                {
                  convert_string_to_vector_of_int(parameter_string2, u1s);
                  t2 = u1s.size();
                  
                  for (i = 0; ok && (i < t2); ++i)
                  {
                    if ((u1s[i] <= 0) || (u1s[i] > t1))
                      ok = false;
                  }
                }
                              
                if (ok)
                {
                  for (i = 0; i < t2; ++i)
                    VEVConfig.SymmetryGroup.observable_sector_U1s.push_back(u1s[i]-1);

                  if (this->print_output)
                  {
                    (*this->Print.out) << "\n  " << this->Print.cbegin << "Observable sector changed." << this->Print.cend;
                    this->Print.PrintGaugeGroup(VEVConfig, true);
                  }
                }
                else
                {
                  if (this->print_output)
                    (*this->Print.out) << "\n  " << this->Print.cbegin << "Observable sector not valid." << this->Print.cend << endl;
                }
              }

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            }


//////////////////// end added select obs sect 7-03           
           

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // analyze the current config
            // updated on 15.07.2011
            if (this->FindCommandType1(command, "analyze config", parameter_string1) || this->FindCommandType1(command, "analyse config", parameter_string1))
            {
              // begin: find parameters
              const bool SM_PrintSU5SimpleRoots = this->FindParameterType1(parameter_string1, "print SU(5) simple roots");

              unsigned NumberOfGenerations = 3;
              this->FindParameterType4(parameter_string1, "generations", NumberOfGenerations);
              // end: find parameters

              bool SM = true;
              bool PS = true;
              bool SU5 = true;

              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Analyzing vev-configuration \"" << VEVConfig.ConfigLabel << VEVConfig.ConfigNumber << "\" ..." << this->Print.cend << "\n";

              vector<SConfig> NewVEVConfigs;
              if (Analyse.AnalyseModel(Orbifold, VEVConfig, SM, PS, SU5, NewVEVConfigs, this->Print, NumberOfGenerations, SM_PrintSU5SimpleRoots))
              {
                t1 = NewVEVConfigs.size();
                for (i = 0; i < t1; ++i)
                {
                  SConfig &NewVEVConfig = NewVEVConfigs[i];
                  while (this->MessageVEVConfigAlreadyExists(VEVConfigs, NewVEVConfig.ConfigLabel, NewVEVConfig.ConfigNumber))
                    ++NewVEVConfig.ConfigNumber;

                  VEVConfigs.push_back(NewVEVConfig);
                }
                // select last config
                VEVConfigIndex = VEVConfigs.size()-1;

                if (this->print_output)
                {
                  if (t1 == 1)
                    (*this->Print.out) << "  " << this->Print.cbegin << "One vev-configuration identified, labeled \"" << VEVConfigs[VEVConfigIndex].ConfigLabel << VEVConfigs[VEVConfigIndex].ConfigNumber << "\" and selected." << this->Print.cend;
                  else
                  {
                    (*this->Print.out) << "\n  " << this->Print.cbegin << t1 << " vev-configurations identified. Print all configurations:" << this->Print.cend << endl;
                    this->ExecuteOrbifoldCommand("print configs");
                    (*this->Print.out) << "  " << this->Print.cbegin << "Now using vev-configuration \"" << VEVConfigs[VEVConfigIndex].ConfigLabel << VEVConfigs[VEVConfigIndex].ConfigNumber << "\"." << this->Print.cend;
                  }
                  this->current_folder[2] = 1;
                  this->ExecuteOrbifoldCommand("print labels");
                  this->current_folder[2] = 0;
                }
              }
              else
              {
                if (this->print_output)
                  (*this->Print.out) << "  " << this->Print.cbegin << "No vev-configuration identified." << this->Print.cend << "\n" << endl;
              }

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            }

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 21.09.2011
            if (print_help)    //menu G
            {
              if (this->FindParameterType1(parameter_string1, "short cuts"))
              {
                (*this->Print.out) << "\n  short cuts:\n";
                (*this->Print.out) << "    m   change directory to /model>\n";
                (*this->Print.out) << "    gg  change directory to /gauge group>\n";
                (*this->Print.out) << "    s   change directory to /spectrum>\n";
                (*this->Print.out) << "    l   change directory to /vev-config/labels>\n\n" << flush;
              }
              else
              {
                (*this->Print.out) << "\n  special commands of this directory:\n";
                (*this->Print.out) << "    use config(ConfigLabel)                   change to configuration \"ConfigLabel\"\n";
                (*this->Print.out) << "    create config(ConfigLabel)                optional: \"from(AnotherConfigLabel)\"\n";
                (*this->Print.out) << "    rename config(OldConfigLabel) to(NewConfigLabel)\n";
                (*this->Print.out) << "    delete config(ConfigLabel)\n\n";
                (*this->Print.out) << "    print configs\n";
                (*this->Print.out) << "    print gauge group\n";

                if (!UsingStandardConfig)
                {
                  t1 = VEVConfig.SymmetryGroup.GaugeGroup.factor.size();
                  t2 = VEVConfig.SymmetryGroup.GaugeGroup.u1directions.size();

                  if ((t1 != 0) || (t2 != 0))
                  {
                    (*this->Print.out) << "\n";
                    if (t1 != 0)
                    {
                      if (t1 == 1)
                      {
                        (*this->Print.out) << "    select observable sector: ...             parameters: \"gauge group(i)\" ";
                        this->PrintFor(t1, "gauge groups", "i");
                      }
                      else
                      if (t1 > 1)
                      {
                        (*this->Print.out) << "    select observable sector: ...             parameters: \"gauge group(i,j,...)\" ";
                        this->PrintFor(t1, "gauge groups", "i,j");
                      }
                      (*this->Print.out) << "                                                          \"full gauge group\"\n";
                      (*this->Print.out) << "                                                          \"no gauge groups\"\n";
                    }
                    if (t2 != 0)
                    {
                      if (t2 == 1)
                      {
                        (*this->Print.out) << "    select observable sector: ...             parameters: \"U1s(i)\" ";
                        this->PrintFor(t2, "U(1)s", "i");
                      }
                      else
                      if (t2 > 1)
                      {
                        (*this->Print.out) << "    select observable sector: ...             parameters: \"U1s(i,j,...)\" ";
                        this->PrintFor(t2, "U(1)s", "i,j");
                      }
                      (*this->Print.out) << "                                                          \"all U1s\"\n";
                      (*this->Print.out) << "                                                          \"no U1s\"\n";
                    }
                  }
                  (*this->Print.out) << "\n";                 
                }
                (*this->Print.out) << "    analyze config                            optional: \"print SU(5) simple roots\"\n";
                (*this->Print.out) << "  change directory:\n";
                (*this->Print.out) << "    cd labels                                 change directory to /labels>\n\n";
                (*this->Print.out) << "  general commands:\n";
                (*this->Print.out) << "    dir                                       show commands\n";
                (*this->Print.out) << "    help                                      optional: \"short cuts\"\n"; 
                (*this->Print.out) << "    cd ..                                     leave this directory\n";
                if (!this->online_mode)
                  (*this->Print.out) << "    exit                                      exit program\n";
                (*this->Print.out) << "\n" << flush;;
              }

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            } // end if print_help
            break;
          } //l
                   
          ////////////////////////////////////////////////////////////////////////////////////////////////////////////
          // labels
          ////////////////////////////////////////////////////////////////////////////////////////////////////////////
          case 1:
          { //m
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 18.02.2011
            if (this->FindCommandType1(command, "cd ..", parameter_string1))
            {
              this->current_folder[2] = 0;

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            }
            
//sept26

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 26.09.2011
            if (this->FindCommandType2(command, "use label(", parameter_string1, parameter_string2))
            {
              if (parameter_string1.find_first_not_of("0123456789") != string::npos)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Label #" << parameter_string1 << " not known." << this->Print.cend << "\n" << endl;
                return true;
              }

              if (UsingStandardConfig)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;
                return true;
              }

              unsigned tmp = (unsigned)atoi(parameter_string1.c_str());
              if ((tmp == 0) || (tmp > Fields[0].Labels.size()))
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Label #" << parameter_string1 << " not known." << this->Print.cend << "\n" << endl;

                return true;
              }

              if (VEVConfig.use_Labels == tmp - 1)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Label #" << parameter_string1 << " is already in use." << this->Print.cend << "\n" << endl;

                return true;
              }

              VEVConfig.use_Labels = tmp - 1;
              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Now using label #" << parameter_string1 << "." << this->Print.cend << "\n" << endl;

              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }

            
//sept25            
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // change the label of a single field
            // updated on 26.09.2011
            if (this->FindCommandType2(command, "change label(", parameter_string1, parameter_string2))
            {
              if (UsingStandardConfig)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;
                return true;
              }

             vector<SUSYMultiplet> Multiplets(2); //added sept25						
	         Multiplets[0]=Scalar;									
	         Multiplets[1]=LeftFermi;       


              if (this->print_output)
                (*this->Print.out) << "\n";

              bool stop = false;

              if (!this->FindParameterType2(parameter_string2, "to(", tmp_string1))
                stop = true;
              else
              {
                vector<string> FieldLabels;
//                ExtractLabels(LeftChiral, parameter_string1, FieldLabels);  //origN=1
                ExtractLabels(Multiplets, parameter_string1, FieldLabels);  //sept25
                vector<unsigned> FieldIndices = GetIndicesOnlyFieldWithNumber(FieldLabels);

                if (FieldIndices.size() != 1)
                  stop = true;

                if (tmp_string1.size() == 0)
                  stop = true;

                string::size_type loc1 = tmp_string1.find("_");
                if (loc1 == string::npos)
                  stop = true;

                unsigned NewIndex = 0;
                if (!stop)
                {
                  tmp_string2 = tmp_string1.substr(0, loc1);
                  tmp_string3 = tmp_string1.substr(loc1+1, string::npos);

                  if (this->MessageLabelError(tmp_string2) || (tmp_string3.size() == 0) || (tmp_string3.find_first_not_of("0123456789") != string::npos))
                    stop = true;
                  else
                  {
                    NewIndex = (unsigned)atoi(tmp_string3.c_str());

                    if (NewIndex == 0)
                    {
                      stop = true;
                      (*this->Print.out) << "  " << this->Print.cbegin << "Label with index 0 not allowed." << this->Print.cend << endl;
                    }
                  }
                }

                const size_t f1 = Fields.size();
                for (i = 0; !stop && (i < f1); ++i)
                {
                  if (i != FieldIndices[0])
                  {
                    const CField &Field = Fields[i];
                    if ((Field.Labels[VEVConfig.use_Labels] == tmp_string2) && (Field.Numbers[VEVConfig.use_Labels] == NewIndex))
                    {
                      if (this->print_output)
                        (*this->Print.out) << "  " << this->Print.cbegin << "Label used by another field." << this->Print.cend << endl;

                      stop = true;
                    }
                  }
                }
                if (!stop)
                {
                  CField &Field = Fields[FieldIndices[0]];
                  Field.Labels[VEVConfig.use_Labels]  = tmp_string2;
                  Field.Numbers[VEVConfig.use_Labels] = NewIndex;

                  if (this->print_output)
                    (*this->Print.out) << "  " << this->Print.cbegin << "Label changed from \"" << parameter_string1 << "\" to \"" << tmp_string2 << "_" << NewIndex << "\"." << this->Print.cend << "\n" << endl;
                }
              }
              if (stop)
              {
                if (this->print_output)
                  (*this->Print.out) << "  " << this->Print.cbegin << "Label not changed." << this->Print.cend << "\n" << endl;
              }

              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }



// begin: create labels , July 15, 2020
//*Help: check canalysemodel.cpp, h.  Labels_Create.
 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // create labels for fields, sorting the fields with respect to the i-th U(1) charge
            // updated on 26.09.2011
            if (this->FindCommandType1(command, "create labels", parameter_string1))
            {

              if (UsingStandardConfig)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;
                return true;
              }            

          vector<SUSYMultiplet> Multiplets(2); //added July 15,2020						
	      Multiplets[0]=Scalar;									
	      Multiplets[1]=LeftFermi;
              

              (*this->Print.out) << "\n";
//              if (!Analyse.Labels_Create(std::cin, VEVConfig, this->Print, LeftChiral, true))
               if (!Analyse.Labels_Create(std::cin, VEVConfig, this->Print, Multiplets, true))
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Creating new labels failed." << this->Print.cend << "\n" << endl;

                return true;
              }
              VEVConfig.use_Labels = Fields[0].Labels.size()-1;

              if (this->print_output)
                (*this->Print.out) << "\n  " << this->Print.cbegin << "Now using new labels (i.e. #" << VEVConfig.use_Labels+1 << ") of the fields." << this->Print.cend << "\n" << endl;

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            }
// end: create labels, July 15, 2020        
        
//sept26

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // assign a label to a fixed point
            // updated on 26.09.2011
            if (this->FindCommandType2(command, "assign label(", parameter_string1, parameter_string2))
            {
              if (UsingStandardConfig)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;
                return true;
              }

              if (this->FindParameterType2(parameter_string2, "to fixed point(", tmp_string1))
              {
                CSpaceGroupElement SGElement;
                if (this->GetLocalization(tmp_string1, SGElement))
                {
                                   
                  bool FixedBraneFound = false;
                  CFixedBrane &FixedBrane = Orbifold.AccessFixedBrane(SGElement, FixedBraneFound);

                  if (FixedBraneFound)
                  {
                                        
                    FixedBrane.AccessFixedBraneLabel() = parameter_string1;

                    if (this->print_output)
                    {
                      (*this->Print.out) << "\n  " << this->Print.cbegin << "Label \"" << FixedBrane.GetFixedBraneLabel() << "\" assigned to fixed point ";
                      this->Print.PrintSGElement(SGElement);
                      (*this->Print.out) << ")." << this->Print.cend << endl;
                    }
                  }
                }
              }
              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }
               
//sept26
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 12.10.2011
            if (this->FindCommandType1(command, "print labels", parameter_string1))
            {
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Using label #" << VEVConfig.use_Labels+1 << " of the fields." << this->Print.cend << "\n" << endl;

//              SUSYMultiplet Multiplet;
//              this->FindSUSYType(parameter_string1, Orbifold.OrbifoldGroup.GetNumberOfSupersymmetry(), Multiplet);

              vector<SUSYMultiplet> Multiplets(2); //sept26
     	      Multiplets[0]=Scalar;								
	          Multiplets[1]=LeftFermi;

//              this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplet, true);
              this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplets, true);
              (*this->Print.out) << endl;

              this->MessageParameterNotKnown(parameter_string1);
              return true;
            }            
            
//sept26
             /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 26.09.2011
            if (this->FindCommandType2(command, "load labels(", parameter_string1, parameter_string2))
            {
              if (UsingStandardConfig)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"StandardConfig1\" cannot be changed." << this->Print.cend << "\n" << endl;
                return true;
              }

            //sept26  
            vector<SUSYMultiplet> Multiplets(2);				//Particle types to be printed
	        Multiplets[0]=Scalar;								//and to be given for equivalence check
	        Multiplets[1]=LeftFermi;  
                
                
              if (this->online_mode)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Command disabled in the web interface." << this->Print.cend << "\n" << endl;
                return true;
              }

              if (Analyse.Labels_Load(parameter_string1, VEVConfig))
              {
                if (this->print_output)
                {
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "New labels loaded. Now using label #" << VEVConfig.use_Labels+1 << " of the fields." << this->Print.cend << "\n" << endl;
 //                 this->Print.PrintSummaryOfVEVConfig(VEVConfig, LeftChiral, true);
                  this->Print.PrintSummaryOfVEVConfig(VEVConfig, Multiplets, true); //sept26
                  (*this->Print.out) << endl;
                }
              }
              else
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "The file \"" << parameter_string1 << "\" does not exist or does not contain valid labels." << this->Print.cend << "\n" << endl;
              }

              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }

//sept26
           /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 30.08.2011
            if (this->FindCommandType2(command, "save labels(", parameter_string1, parameter_string2))
            {
              if (this->online_mode)
              {
                if (this->print_output)
                  (*this->Print.out) << "\n  " << this->Print.cbegin << "Command disabled in the web interface." << this->Print.cend << "\n" << endl;
                return true;
              }

              std::ofstream out(parameter_string1.data());
              if((!out.is_open()) || (!out.good()))
              {
                (*this->Print.out) << "\n  " << this->Print.cbegin << "File \"" << parameter_string1 << "\" not found." << this->Print.cend << "\n" << endl;

                this->MessageParameterNotKnown(parameter_string2);
                return true;
              }

              const size_t f1 = Fields.size();
              for (i = 0; i < f1; ++i)
              {
                if (i != 0) out << "\n";
                out << i << " " << Fields[i].Labels[VEVConfig.use_Labels] << "_" << Fields[i].Numbers[VEVConfig.use_Labels];
              }
              out.close();

              if (this->print_output)
                (*this->Print.out) << "  " << this->Print.cbegin << f1 << " labels saved to file \"" << parameter_string1 << "\"." << this->Print.cend << "\n" << endl;

              this->MessageParameterNotKnown(parameter_string2);
              return true;
            }


//sept26          
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // updated on 26.09.2011
            if (this->FindCommandType1(command, "dir", parameter_string1) || this->FindCommandType1(command, "help", parameter_string1) || this->FindCommandType1(command, "ll", parameter_string1))
            { //begin dir
              if (this->FindParameterType1(parameter_string1, "short cuts"))
              {
                (*this->Print.out) << "\n  short cuts:\n";
                (*this->Print.out) << "    m   change directory to /model>\n";
                (*this->Print.out) << "    gg  change directory to /gauge group>\n";
                (*this->Print.out) << "    s   change directory to /spectrum>\n";
                (*this->Print.out) << "    v   change directory to /vev-config>\n\n" << flush;
              }
              else
              {
                (*this->Print.out) << "\n  special commands of this directory:\n";
                if (!UsingStandardConfig)
                {
                  (*this->Print.out) << "    change label(A_i) to(B_j)\n";
                  (*this->Print.out) << "    create labels\n";
                  (*this->Print.out) << "    assign label(Label) to fixed point(k,l,n1,n2,n3,n4,n5,n6)\n";
                }
                (*this->Print.out) << "    print labels                              \n";
                if (!UsingStandardConfig)
                {
                  (*this->Print.out) << "    use label(i)                              ";

                  const size_t f1 = VEVConfig.Fields.size();
                  if (f1 == 0)
                  {
                    cout << "Warning in bool CPrompt::ExecuteCommand(...): \"Fields\" is empty. Return true." << endl;
                    return true;  
                  }
                  this->PrintFor(VEVConfig.Fields[0].Labels.size(), "labels", "i");
                  (*this->Print.out) << "\n";
                }
                if (!this->online_mode)
                {
                  if (!UsingStandardConfig)
                    (*this->Print.out) << "    load labels(Filename)\n";
                  (*this->Print.out) << "    save labels(Filename)\n";
                }
                (*this->Print.out) << "\n";
                (*this->Print.out) << "  general commands:\n";
                (*this->Print.out) << "    dir                                       show commands\n";
                (*this->Print.out) << "    help                                      optional: \"short cuts\"\n";
                (*this->Print.out) << "    cd ..                                     leave this directory\n";
                if (!this->online_mode)
                  (*this->Print.out) << "    exit                                      exit program\n";
                (*this->Print.out) << "\n" << flush;;
              }
              this->MessageParameterNotKnown(parameter_string1);
              return true;
            } //end dir
            //here
            break;
          } //m
        } //k
        break;
      }  // j    
//end put case 5 vev-config from U1
    } // d  end
    // end: commands available in sub folders
    // STEP 5 //////////////////////////////////////////////////////////////////////////////////////////////////////////
  }  // c  end
  this->MessageParameterNotKnown(command);
  return true;
} // a  end


/* ########################################################################################
######   LoadProgram(const string &Filename, vector<string> &Commands)               ######
######                                                                               ######
######   Version: 03.05.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Filename  : filename of the script file                                  ######
######   output:                                                                     ######
######   2) Commands  : list of commanmds read from "Filename"                       ######
######   return value : finished successfully?                                       ######
###########################################################################################
######   description:                                                                ######
######   Reads commands from file "Filename" and stores them in "Commands".          ######
######################################################################################## */
bool CPrompt::LoadProgram(const string &Filename, vector<string> &Commands)
{
  struct stat stFileInfo;
  int intStat = stat(Filename.c_str(),&stFileInfo);
  if (intStat == 0)
  {
    usleep(200);
    std::ifstream input(Filename.data());

    string command = "";
    unsigned counter = 0;
    while (GetSaveLine(input, command))
    {
      ++counter;
      Commands.push_back(command);
    }
    input.close();
    (*this->Print.out) << "\n  " << this->Print.cbegin << counter << " commands loaded from file \"" << Filename << "\"." << this->Print.cend << "\n" << endl;
  }
  return true;
}



/* ########################################################################################
######   LoadOrbifolds(const string &Filename, bool inequivalent, ...)               ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Filename                      : filename of the model file               ######
######   2) inequivalent                  : only load inequivalent orbifold models   ######
######   3) compare_couplings_up_to_order : refines the comparision method           ######
######   output:                                                                     ######
######   return value                     : finished successfully?                   ######
###########################################################################################
######   description:                                                                ######
######   Load orbifold models from file "Filename" into the prompt.                  ######
######################################################################################## */
/*bool CPrompt::LoadOrbifolds(const string &Filename, bool inequivalent, unsigned compare_couplings_up_to_order)
{
/*  const bool compare_couplings = (inequivalent && (compare_couplings_up_to_order >= 3));

  if (this->print_output)
  {
    (*this->Print.out) << "  " << this->Print.cbegin << "Load ";
    if (inequivalent)
      (*this->Print.out) << "inequivalent ";
    (*this->Print.out) << "orbifolds from file \"" << Filename << "\"";
    if (compare_couplings)
      (*this->Print.out) << " (using the number of couplings of order " << compare_couplings_up_to_order << " for comparison)";
    (*this->Print.out) << "." << this->Print.cend << flush;
  }

  string ProgramFilename = "";
  std::ifstream in(Filename.data());
  if((!in.is_open()) || (!in.good()))
  {
    if (this->print_output)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "File \"" << Filename << "\" not found." << this->Print.cend << "\n" << endl;
    return false;
  }

  CInequivalentModels Models_01;
  vector<string> FieldLabels;
  unsigned j = 0;

  const size_t o1 = this->Orbifolds.size();
  if (inequivalent)
  {
    for (unsigned i = 0; i < o1; ++i)
    {
      COrbifold &Orbifold = this->Orbifolds[i];
      CSpectrum Spectrum(Orbifold.StandardConfig, LeftChiral);

      if (compare_couplings)
      {
        SConfig tmp_VEVConfig = Orbifold.StandardConfig;

        for (j = 3; j <= compare_couplings_up_to_order; ++j)
        {
          FieldLabels.assign(j, "*");
          Orbifold.YukawaCouplings.AddCoupling(Orbifold, tmp_VEVConfig, FieldLabels);
        }

        Spectrum.AddIdentifier(tmp_VEVConfig.FieldCouplings.size());
      }

      Models_01.IsSpectrumUnknown(Spectrum, true);
    }
  }

  unsigned counter = 0;
  const unsigned MAXcounter = 2000;
  bool Orbifold_loaded = true;

  unsigned NewNumber = 1;
  string NewLabelPart1 = "";
  string NewLabel = "";

  COrbifoldGroup NewOrbifoldGroup;
  while ((counter < MAXcounter) && !in.eof())
  {
    if (NewOrbifoldGroup.LoadOrbifoldGroup(in, ProgramFilename))
    {
      // begin: for models randomly created
      if ((NewOrbifoldGroup.Label.substr(0,6) == "Random") && this->MessageOrbifoldAlreadyExists(NewOrbifoldGroup.Label, false))
      {
        NewNumber = 1;
        NewLabelPart1 = NewOrbifoldGroup.Label;
        this->SplitVEVConfigLabel(NewLabelPart1, NewNumber);
        
        ++NewNumber;
        ostringstream os;
        os << NewNumber;
        NewLabel = NewLabelPart1 + os.str();

        while (this->MessageOrbifoldAlreadyExists(NewLabel, false))
        {
          ++NewNumber; 
          ostringstream os;
          os << NewNumber;
          NewLabel = NewLabelPart1 + os.str();
        }
        NewOrbifoldGroup.Label = NewLabel;
      }
      // end: for models randomly created

      if (!this->MessageOrbifoldAlreadyExists(NewOrbifoldGroup.Label))
      {
        COrbifold NewOrbifold(NewOrbifoldGroup);
        NewOrbifold.CheckAnomaly(NewOrbifold.StandardConfig, this->GaugeIndices, this->Print, false);
        
        /*SConfig VEVConfig = NewOrbifold.StandardConfig;
        VEVConfig.SymmetryGroup.observable_sector_U1s.clear();
        this->Print.PrintSummaryOfVEVConfig(VEVConfig, LeftChiral, false);
        CSpaceGroupElement anomalous_element;
        NewOrbifold.CheckDiscreteAnomaly(NewOrbifold.StandardConfig, this->GaugeIndices, anomalous_element, this->Print, true);*/


/*        Orbifold_loaded = true;

        if (inequivalent)
        {
          CSpectrum Spectrum(NewOrbifold.StandardConfig, LeftChiral);

          if (compare_couplings)
          {
            SConfig VEVConfig = NewOrbifold.StandardConfig;

            for (j = 3; j <= compare_couplings_up_to_order; ++j)
            {
              FieldLabels.assign(j, "*");
              NewOrbifold.YukawaCouplings.AddCoupling(NewOrbifold, VEVConfig, FieldLabels);
            }

            Spectrum.AddIdentifier(VEVConfig.FieldCouplings.size());
          }
          Orbifold_loaded = Models_01.IsSpectrumUnknown(Spectrum, true);
        }

        if (Orbifold_loaded)
        {
          this->Orbifolds.push_back(NewOrbifold);
          ++counter;

          SConfig TestConfig = NewOrbifold.StandardConfig;
          TestConfig.ConfigLabel = "TestConfig";
          TestConfig.ConfigNumber = 1;

          vector<SConfig> Configs;
          Configs.push_back(NewOrbifold.StandardConfig);
          Configs.push_back(TestConfig);
          this->AllVEVConfigs.push_back(Configs);
          this->AllVEVConfigsIndex.push_back(1);

          if (ProgramFilename != "")
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Execute program from file \"" << ProgramFilename << "\" for orbifold \"" << NewOrbifoldGroup.Label << "\"." << this->Print.cend << flush;
            this->OrbifoldIndex = this->Orbifolds.size() - 1;
            this->current_folder.assign(10,0);
            this->current_folder[0]  = this->OrbifoldIndex;
            this->current_folder[1]  = 0;
            this->current_folder[2]  = 0;

            this->StartPrompt(ProgramFilename, true, false);
          }
        }
      }
    }
  }
  in.close();

  this->current_folder.assign(10,0);
  this->current_folder[0]  = -1;
  this->current_folder[1]  = 0;
  this->current_folder[2]  = 0;

  if (this->print_output)
  {
    if (this->Orbifolds.size() == MAXcounter)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "Maximal limit of " << MAXcounter << " orbifold models reached." << this->Print.cend;
    else
    {
      if (counter == 0)
        (*this->Print.out) << "\n  " << this->Print.cbegin << "No orbifolds loaded." << this->Print.cend;
      else
        if (counter == 1)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold \"" << this->Orbifolds[this->Orbifolds.size() - 1].OrbifoldGroup.Label << "\" loaded." << this->Print.cend;
      else
        (*this->Print.out) << "\n  " << this->Print.cbegin << counter << " orbifolds loaded." << this->Print.cend;
    }
    (*this->Print.out) << "\n\n";
  }
  (*this->Print.out) << flush;
  return true; */
/*}*/

///////// begin LoadOrbifolds from U1s   (original before april26)

/* TD
bool CPrompt::LoadOrbifolds(const string &Filename, bool inequivalent, unsigned compare_couplings_up_to_order)
{
//  const bool compare_couplings = (inequivalent && (compare_couplings_up_to_order >= 3));

/* if (this->print_output)
  {
    (*this->Print.out) << "  " << this->Print.cbegin << "Load ";
    if (inequivalent)
      (*this->Print.out) << "inequivalent ";
    (*this->Print.out) << "orbifolds from file \"" << Filename << "\"";
    if (compare_couplings)
      (*this->Print.out) << " (using the number of couplings of order " << compare_couplings_up_to_order << " for comparison)";
    (*this->Print.out) << "." << this->Print.cend << flush;
  }*/
/*TD
  string ProgramFilename = "";
  std::ifstream in(Filename.data());
  if((!in.is_open()) || (!in.good()))
  {
    if (this->print_output)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "File \"" << Filename << "\" not found." << this->Print.cend << "\n" << endl;
    return false;
  }

  CInequivalentModels Models_01;
  vector<string> FieldLabels;
  unsigned j = 0;

  const size_t o1 = this->Orbifolds.size();
/*  if (inequivalent)
  {
    for (unsigned i = 0; i < o1; ++i)
    {
      COrbifold &Orbifold = this->Orbifolds[i];
      CSpectrum Spectrum(Orbifold.StandardConfig, LeftChiral);

      if (compare_couplings)
      {
        SConfig tmp_VEVConfig = Orbifold.StandardConfig;

        for (j = 3; j <= compare_couplings_up_to_order; ++j)
        {
          FieldLabels.assign(j, "*");
          Orbifold.YukawaCouplings.AddCoupling(Orbifold, tmp_VEVConfig, FieldLabels);
        }

        Spectrum.AddIdentifier(tmp_VEVConfig.FieldCouplings.size());
      }

      Models_01.IsSpectrumUnknown(Spectrum, true);
    }
  }*/
/*TD
  unsigned counter = 0;
  const unsigned MAXcounter = 2000;  //Here Dec23  (orig 2000)
  bool Orbifold_loaded = true;

  unsigned NewNumber = 1;
  string NewLabelPart1 = "";
  string NewLabel = "";

  COrbifoldGroup NewOrbifoldGroup;
//  NewOrbifoldGroup.LoadOrbifoldGroup(in, ProgramFilename);  // added by me
  
  while ((counter < MAXcounter) && !in.eof())
  {
    if (NewOrbifoldGroup.LoadOrbifoldGroup(in, ProgramFilename))
    {
      // begin: for models randomly created
      /*if ((NewOrbifoldGroup.Label.substr(0,6) == "Random") && this->MessageOrbifoldAlreadyExists(NewOrbifoldGroup.Label, false))
      {
        NewNumber = 1;
        NewLabelPart1 = NewOrbifoldGroup.Label;
        this->SplitVEVConfigLabel(NewLabelPart1, NewNumber);
        
        ++NewNumber;
        ostringstream os;
        os << NewNumber;
        NewLabel = NewLabelPart1 + os.str();

        while (this->MessageOrbifoldAlreadyExists(NewLabel, false))
        {
          ++NewNumber; 
          ostringstream os;
          os << NewNumber;
          NewLabel = NewLabelPart1 + os.str();
        }
        NewOrbifoldGroup.Label = NewLabel;
      }*/
      // end: for models randomly created
/*TD
      if (!this->MessageOrbifoldAlreadyExists(NewOrbifoldGroup.Label))
      {
        COrbifold NewOrbifold(NewOrbifoldGroup);
        NewOrbifold.CheckAnomaly(NewOrbifold.StandardConfig, this->GaugeIndices, this->Print, false);
        
        /*SConfig VEVConfig = NewOrbifold.StandardConfig;
        VEVConfig.SymmetryGroup.observable_sector_U1s.clear();
        this->Print.PrintSummaryOfVEVConfig(VEVConfig, LeftChiral, false);
        CSpaceGroupElement anomalous_element;
        NewOrbifold.CheckDiscreteAnomaly(NewOrbifold.StandardConfig, this->GaugeIndices, anomalous_element, this->Print, true);*/
/*TD
        Orbifold_loaded = true;

        /*if (inequivalent)
        {
          CSpectrum Spectrum(NewOrbifold.StandardConfig, LeftChiral);

          if (compare_couplings)
          {
            SConfig VEVConfig = NewOrbifold.StandardConfig;

            for (j = 3; j <= compare_couplings_up_to_order; ++j)
            {
              FieldLabels.assign(j, "*");
              NewOrbifold.YukawaCouplings.AddCoupling(NewOrbifold, VEVConfig, FieldLabels);
            }

            Spectrum.AddIdentifier(VEVConfig.FieldCouplings.size());
          }
          Orbifold_loaded = Models_01.IsSpectrumUnknown(Spectrum, true);
        }*/
/*TD
        if (Orbifold_loaded)
        {
          this->Orbifolds.push_back(NewOrbifold);
          ++counter;

          SConfig TestConfig = NewOrbifold.StandardConfig;
          TestConfig.ConfigLabel = "TestConfig";
          TestConfig.ConfigNumber = 1;

          vector<SConfig> Configs;
          Configs.push_back(NewOrbifold.StandardConfig);
          Configs.push_back(TestConfig);
          this->AllVEVConfigs.push_back(Configs);
          this->AllVEVConfigsIndex.push_back(1);

          /*if (ProgramFilename != "")
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Execute program from file \"" << ProgramFilename << "\" for orbifold \"" << NewOrbifoldGroup.Label << "\"." << this->Print.cend << flush;
            this->OrbifoldIndex = this->Orbifolds.size() - 1;
            this->current_folder.assign(10,0);
            this->current_folder[0]  = this->OrbifoldIndex;
            this->current_folder[1]  = 0;
            this->current_folder[2]  = 0;

            this->StartPrompt(ProgramFilename, true, false);
          }*/
/*TD        }
      }
    }
  }
  in.close();

  this->current_folder.assign(10,0);
  this->current_folder[0]  = -1;
  this->current_folder[1]  = 0;
  this->current_folder[2]  = 0;

  if (this->print_output)
  {
    if (this->Orbifolds.size() == MAXcounter)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "Maximal limit of " << MAXcounter << " orbifold models reached." << this->Print.cend;
    else
    {
      if (counter == 0)
        (*this->Print.out) << "\n  " << this->Print.cbegin << "No orbifolds loaded." << this->Print.cend;
      else
        if (counter == 1)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold \"" << this->Orbifolds[this->Orbifolds.size() - 1].OrbifoldGroup.Label << "\" loaded." << this->Print.cend;
      else
        (*this->Print.out) << "\n  " << this->Print.cbegin << counter << " orbifolds loaded." << this->Print.cend;
    }
    (*this->Print.out) << "\n\n";
  }
  (*this->Print.out) << flush;
  return true; 
}
*/ //TD

///////// end LoadOrbifolds from U1s   (original before aoril26)


//// begin new LoadOrbifolds, april 26
bool CPrompt::LoadOrbifolds(const string &Filename, bool inequivalent, unsigned compare_couplings_up_to_order)
{
//  const bool compare_couplings = (inequivalent && (compare_couplings_up_to_order >= 3));

 if (this->print_output)
  {
    (*this->Print.out) << "  " << this->Print.cbegin << "Load ";
    if (inequivalent)
      (*this->Print.out) << "inequivalent ";
    (*this->Print.out) << "orbifolds from file \"" << Filename << "\"";
//    if (compare_couplings)
//      (*this->Print.out) << " (using the number of couplings of order " << compare_couplings_up_to_order << " for comparison)";
    (*this->Print.out) << "." << this->Print.cend << flush;
  }

  string ProgramFilename = "";
  std::ifstream in(Filename.data());
  if((!in.is_open()) || (!in.good()))
  {
    if (this->print_output)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "File \"" << Filename << "\" not found." << this->Print.cend << "\n" << endl;
    return false;
  }

//////////begin mult added april26

    vector<SUSYMultiplet> Multiplets(2);						//Particle types to be printed
	Multiplets[0]=Scalar;									//and to be given for equivalence check
	Multiplets[1]=LeftFermi;
//////////end mult added april26



//  CInequivalentModels Models_01;
  CInequivalentModels InequivModels; //added april26
  vector<string> FieldLabels; 
  unsigned j = 0;

  const size_t o1 = this->Orbifolds.size();    //april 26 , try if we comment this block also. it remains and it works m>1 models.
  if (inequivalent)
  {
    for (unsigned i = 0; i < o1; ++i)
    {
      COrbifold &Orbifold = this->Orbifolds[i];
//      CSpectrum Spectrum(Orbifold.StandardConfig, LeftChiral); 
      CSpectrum Spectrum(Orbifold.StandardConfig, Multiplets); //added april26
      
/*      if (compare_couplings)
      {
        SConfig tmp_VEVConfig = Orbifold.StandardConfig;

        for (j = 3; j <= compare_couplings_up_to_order; ++j)
        {
          FieldLabels.assign(j, "*");
          Orbifold.YukawaCouplings.AddCoupling(Orbifold, tmp_VEVConfig, FieldLabels);
        }

        Spectrum.AddIdentifier(tmp_VEVConfig.FieldCouplings.size());
      }
*/
//      Models_01.IsSpectrumUnknown(Spectrum, true);
      InequivModels.IsSpectrumUnknown(Spectrum, true); // added april26      
    }
  }

  unsigned counter = 0;
  const unsigned MAXcounter = 2000;  //Here Dec23  (orig 2000)
  bool Orbifold_loaded = true;

  unsigned NewNumber = 1;
  string NewLabelPart1 = "";
  string NewLabel = "";

  COrbifoldGroup NewOrbifoldGroup;
//  NewOrbifoldGroup.LoadOrbifoldGroup(in, ProgramFilename);  // added by me
  
  while ((counter < MAXcounter) && !in.eof())
  {
    if (NewOrbifoldGroup.LoadOrbifoldGroup(in, ProgramFilename))
    {
      // begin: for models randomly created
      /*if ((NewOrbifoldGroup.Label.substr(0,6) == "Random") && this->MessageOrbifoldAlreadyExists(NewOrbifoldGroup.Label, false))
      {
        NewNumber = 1;
        NewLabelPart1 = NewOrbifoldGroup.Label;
        this->SplitVEVConfigLabel(NewLabelPart1, NewNumber);
        
        ++NewNumber;
        ostringstream os;
        os << NewNumber;
        NewLabel = NewLabelPart1 + os.str();

        while (this->MessageOrbifoldAlreadyExists(NewLabel, false))
        {
          ++NewNumber; 
          ostringstream os;
          os << NewNumber;
          NewLabel = NewLabelPart1 + os.str();
        }
        NewOrbifoldGroup.Label = NewLabel;
      }*/
      // end: for models randomly created

      if (!this->MessageOrbifoldAlreadyExists(NewOrbifoldGroup.Label))
      {
        COrbifold NewOrbifold(NewOrbifoldGroup);
        NewOrbifold.CheckAnomaly(NewOrbifold.StandardConfig, this->GaugeIndices, this->Print, false);
        
        /*SConfig VEVConfig = NewOrbifold.StandardConfig;
        VEVConfig.SymmetryGroup.observable_sector_U1s.clear();
        this->Print.PrintSummaryOfVEVConfig(VEVConfig, LeftChiral, false);
        CSpaceGroupElement anomalous_element;
        NewOrbifold.CheckDiscreteAnomaly(NewOrbifold.StandardConfig, this->GaugeIndices, anomalous_element, this->Print, true);*/

        Orbifold_loaded = true;

        if (inequivalent)
        {
//          CSpectrum Spectrum(NewOrbifold.StandardConfig, LeftChiral);
          CSpectrum Spectrum(NewOrbifold.StandardConfig, Multiplets); // added april 26

/*          if (compare_couplings)
          {
            SConfig VEVConfig = NewOrbifold.StandardConfig;

            for (j = 3; j <= compare_couplings_up_to_order; ++j)
            {
              FieldLabels.assign(j, "*");
              NewOrbifold.YukawaCouplings.AddCoupling(NewOrbifold, VEVConfig, FieldLabels);
            }

            Spectrum.AddIdentifier(VEVConfig.FieldCouplings.size());
          }
*/
//          Orbifold_loaded = Models_01.IsSpectrumUnknown(Spectrum, true);
          Orbifold_loaded = InequivModels.IsSpectrumUnknown(Spectrum, true); //added april 26         
        }

        if (Orbifold_loaded)
        {
          this->Orbifolds.push_back(NewOrbifold);
          ++counter;

          SConfig TestConfig = NewOrbifold.StandardConfig;
          TestConfig.ConfigLabel = "TestConfig";
          TestConfig.ConfigNumber = 1;

          vector<SConfig> Configs;
          Configs.push_back(NewOrbifold.StandardConfig);
          Configs.push_back(TestConfig);
          this->AllVEVConfigs.push_back(Configs);
          this->AllVEVConfigsIndex.push_back(1);

          /*if (ProgramFilename != "")
          {
            if (this->print_output)
              (*this->Print.out) << "\n  " << this->Print.cbegin << "Execute program from file \"" << ProgramFilename << "\" for orbifold \"" << NewOrbifoldGroup.Label << "\"." << this->Print.cend << flush;
            this->OrbifoldIndex = this->Orbifolds.size() - 1;
            this->current_folder.assign(10,0);
            this->current_folder[0]  = this->OrbifoldIndex;
            this->current_folder[1]  = 0;
            this->current_folder[2]  = 0;

            this->StartPrompt(ProgramFilename, true, false);
          }*/
        }
      }
    }
  }
  in.close();

  this->current_folder.assign(10,0);
  this->current_folder[0]  = -1;
  this->current_folder[1]  = 0;
  this->current_folder[2]  = 0;

  if (this->print_output)
  {
    if (this->Orbifolds.size() == MAXcounter)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "Maximal limit of " << MAXcounter << " orbifold models reached." << this->Print.cend;
    else
    {
      if (counter == 0)
        (*this->Print.out) << "\n  " << this->Print.cbegin << "No orbifolds loaded." << this->Print.cend;
      else
        if (counter == 1)
          (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold \"" << this->Orbifolds[this->Orbifolds.size() - 1].OrbifoldGroup.Label << "\" loaded." << this->Print.cend;
      else
        (*this->Print.out) << "\n  " << this->Print.cbegin << counter << " orbifolds loaded." << this->Print.cend;
    }
    (*this->Print.out) << "\n\n";
  }
  (*this->Print.out) << flush;
  return true; 
}
//// end new LoadOrbifolds, april 26




/* ########################################################################################
######   FindCommandType0(const string &inputstring, const string &command) const    ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) inputstring : contains the command given by the user                     ######
######   2) command     : search for this command                                    ######
######   output:                                                                     ######
######   return value   : execute "command"?                                         ######
###########################################################################################
######   description:                                                                ######
######   Compares "command" and the input "inputstring".                             ######
######################################################################################## */
bool CPrompt::FindCommandType0(const string &inputstring, const string &command) const
{
  if (inputstring == command)
    return true;

  return false;
}



/* ########################################################################################
######   FindCommandType1(const string &inputstring, ...) const                      ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) inputstring : contains the command given by the user                     ######
######   2) command     : search for this command                                    ######
######   output:                                                                     ######
######   3) parameters  : the part of "inputstring" not being "command" is stored as ######
######                    "parameters"                                               ######
######   return value   : execute "command"?                                         ######
###########################################################################################
######   description:                                                                ######
######   Compares "command" and the input "inputstring".                             ######
######################################################################################## */
bool CPrompt::FindCommandType1(const string &inputstring, const string &command, string &parameters) const
{
  size_t c1 = command.size();

  if (inputstring.substr(0, c1) == command)
  {
    parameters = inputstring.substr(c1, string::npos);
    return true;
  }
  return false;
}



/* ########################################################################################
######   FindCommandType2(const string &inputstring, ...) const                      ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) inputstring       : contains the command given by the user               ######
######   2) command           : search for this command                              ######
######   output:                                                                     ######
######   3) command_parameter : see example                                          ######
######   4) other_parameters  : see example                                          ######
######   return value         : execute "command"?                                   ######
###########################################################################################
######   description:                                                                ######
######   Example: inputstring = "print(something) with something else"               ######
######   then: command = "print("                                                    ######
######         command_parameter = "something"                                       ######
######         other_parameters = "with something else"                              ######
######################################################################################## */
bool CPrompt::FindCommandType2(const string &inputstring, const string &command, string &command_parameter, string &other_parameters) const
{
  string::size_type loc1 = inputstring.find_first_not_of(" ");
  if (loc1 == string::npos)
    return false;

  string::size_type loc2 = 0;

  if (inputstring.substr(loc1, command.size()) == command)
  {
    loc1 = inputstring.find("(", 0);
    if (loc1 != string::npos)
    {
      loc2 = inputstring.find( ")", loc1);
      if (loc2 != string::npos)
        command_parameter = inputstring.substr(loc1+1, loc2-loc1-1);
      else
        return false;
    }
    else
      return false;

    other_parameters = inputstring.substr(loc2+1, string::npos);

    return true;
  }
  return false;
}



/* ########################################################################################
######   FindParameterType1(string &inputstring, const string &parameter) const      ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) inputstring       : contains the parameter given by the user             ######
######   2) parameter         : search for this parameter                            ######
######   output:                                                                     ######
######   return value         : "parameter" contained in "inputstring"?              ######
###########################################################################################
######   description:                                                                ######
######   If "parameter" is contained in "inputstring" remove it and return true.     ######
######################################################################################## */
bool CPrompt::FindParameterType1(string &inputstring, const string &parameter) const
{
  string::size_type loc1 = inputstring.find(parameter, 0);
  if (loc1 != string::npos)
  {
    string tmp = inputstring;
    inputstring = tmp.substr(0, loc1);
    inputstring += tmp.substr(loc1 + parameter.size(), string::npos);

    return true;
  }
  return false;
}



/* ########################################################################################
######   FindParameterType2(string &inputstring, const string &parameter, ...) const ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) inputstring     : contains the parameter given by the user               ######
######   2) parameter       : search for this parameter                              ######
######   output:                                                                     ######
######   3) parameter_value : see example                                            ######
######   return value       : "parameter" contained in "inputstring"?                ######
###########################################################################################
######   description:                                                                ######
######   If "parameter" is contained in "inputstring" remove it and return true.     ######
######   Example: inputstring = "if(Q_4 == even) and more"                           ######
######   then: parameter = "if("                                                     ######
######         parameter_value = "Q_4 == even"                                       ######
######         inputstring = "and more"                                              ######
######################################################################################## */
bool CPrompt::FindParameterType2(string &inputstring, const string &parameter, string &parameter_value) const
{
  size_t c1 = parameter.size();

  string::size_type loc1 = inputstring.find(parameter, 0);
  if (loc1 != string::npos)
  {
    string::size_type loc2 = inputstring.find(")", loc1+c1);
    if (loc2 != string::npos)
    {
      parameter_value = inputstring.substr(loc1+c1, loc2-loc1-c1);
      string tmp = inputstring;
      inputstring = tmp.substr(0, loc1);
      inputstring += tmp.substr(loc2 + 1, string::npos);
      return true;
    }
  }
  return false;
}



/* ########################################################################################
######   FindParameterType3(string &inputstring, const string &parameter, ...) const ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) inputstring : contains the parameter given by the user                   ######
######   2) parameter   : search for this parameter                                  ######
######   output:                                                                     ######
######   3) index       : see example                                                ######
######   return value   : "parameter" contained in "inputstring"?                    ######
###########################################################################################
######   description:                                                                ######
######   If "parameter" is contained in "inputstring" remove it and return true.     ######
######   Example: inputstring = "Q_4 == even"                                        ######
######   then: parameter = "Q_"                                                      ######
######         index = 4                                                             ######
######         inputstring = "== even"                                               ######
######################################################################################## */
bool CPrompt::FindParameterType3(string &inputstring, const string &parameter, unsigned &index) const
{
  size_t c1 = parameter.size();

  string::size_type loc1 = inputstring.find(parameter, 0);
  if (loc1 != string::npos)
  {
    string::size_type loc2 = inputstring.find(" ", loc1+c1);
    if (loc2 != string::npos)
    {
      index = atoi(inputstring.substr(loc1+c1, loc2-loc1-c1).c_str());
      string tmp = inputstring;
      inputstring = tmp.substr(0, loc1);
      inputstring += tmp.substr(loc2, string::npos);
      return true;
    }
  }
  return false;
}



/* ########################################################################################
######   FindParameterType4(string &inputstring, const string &parameter, ...) const ######
######                                                                               ######
######   Version: 06.01.2012                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) inputstring : contains the parameter given by the user                   ######
######   2) parameter   : search for this parameter                                  ######
######   output:                                                                     ######
######   3) number      : see example                                                ######
######   return value   : "parameter" contained in "inputstring"?                    ######
###########################################################################################
######   description:                                                                ######
######   If "parameter" is contained in "inputstring" remove it and return true.     ######
######   Example: inputstring = "3generations"                                       ######
######   then: parameter = "generations"                                             ######
######         number = 3                                                            ######
######         inputstring = ""                                                      ######
######################################################################################## */
bool CPrompt::FindParameterType4(string &inputstring, const string &parameter, unsigned &number) const
{
  string::size_type loc1 = inputstring.find(parameter, 0);
  if (loc1 != string::npos)
  {
    int loc2 = loc1-1;
    while (loc2 >= 0)
    {
      if ((inputstring.substr(loc2,1)).find_first_of("1234567890") == string::npos)
        break;

      --loc2;
    }
    string tmp = inputstring.substr(loc2+1,loc1-loc2-1);
    if ((tmp.size() == 0) || (tmp.find_first_not_of("1234567890") != string::npos))
      return false;

    number = atoi(tmp.c_str());

    tmp = inputstring;
    inputstring = tmp.substr(0, loc1);
    inputstring += tmp.substr(loc1 + parameter.size(), string::npos);

    return true;
  }
  return false;
}



/* ########################################################################################
######   MessageHelpCreateNewOrbifold(unsigned StartWithLine) const                  ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) StartWithLine : write the message starting from the "StartWithLine"-th   ######
######                      line                                                     ######
######   output:                                                                     ######
######   -                                                                           ######
###########################################################################################
######   description:                                                                ######
######   Displays a help explaining how to create a new orbifold model.              ######
######################################################################################## */
void CPrompt::MessageHelpCreateNewOrbifold(unsigned StartWithLine) const
{
  if (!this->print_output)
    return;

  (*this->Print.out) << "\n  " << this->Print.cbegin << "Input data for orbifold model \"" << this->Orbifolds[this->OrbifoldIndex].OrbifoldGroup.Label << "\" is needed:" << this->Print.cend << "\n";

  vector<string> Text;
  //Text.push_back(") set heterotic string type(type) : type = \"E8xE8\" or \"Spin32\".");  

  Text.push_back(") heterotic string type SO(16)xSO(16) is already assigned.");  //june27

  Text.push_back(") print available space groups    : Print a list of possible space groups.");
  Text.push_back(") use space group(i)              : Choose the space group.");

  string SetShift = ") set shift standard embedding      or";
  if (this->Orbifolds[this->OrbifoldIndex].OrbifoldGroup.GetOrderZN() != 1)
    SetShift += this->Print.cend + "\n  " + this->Print.cbegin + "     set shift V(i) = <16D vector>   : Set the gauge shift.";
  else
    SetShift += this->Print.cend + "\n  " + this->Print.cbegin + "     set shift V = <16D vector>      : Set the gauge shift.";
  Text.push_back(SetShift);

  Text.push_back(") set WL W(i) = <16D vector>      : Set the i-th Wilson line.");
  const size_t t1 = Text.size();

  for (unsigned i = StartWithLine; i < t1; ++i)
    (*this->Print.out) << "  " << this->Print.cbegin << "  " << i+1-StartWithLine << Text[i] << this->Print.cend << "\n";

  (*this->Print.out) << endl;
}



/* ########################################################################################
######   MessageLabelError(const string &Label) const                                ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Label     : label of, for example, a field or orbifold to be checked for ######
######                  correctness                                                  ######
######   output:                                                                     ######
######   return value : is the label correct?                                        ######
###########################################################################################
######   description:                                                                ######
######   Checks whether "Label" is correct and displays a message if not.            ######
######################################################################################## */
bool CPrompt::MessageLabelError(const string &Label) const
{
  if ((Label.size() == 0) || (Label.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != string::npos))
  {
    if (this->print_output)
      (*this->Print.out) << "  " << this->Print.cbegin << "Label Error: The label is only allowed to contain characters and numbers." << this->Print.cend << endl;
    return true;
  }
  return false;
}



/* ########################################################################################
######   MessageOrbifoldAlreadyExists(const string &OrbifoldLabel, ...) const        ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) OrbifoldLabel : label of an orbifold be checked                          ######
######   output:                                                                     ######
######   return value     : does an orbifold with label "OrbifoldLabel" already      ######
######                      exist?                                                   ######
###########################################################################################
######   description:                                                                ######
######   Checks whether "OrbifoldLabel" already exists.                              ######
######################################################################################## */
bool CPrompt::MessageOrbifoldAlreadyExists(const string &OrbifoldLabel, bool PrintOutput) const
{
  size_t s1 = this->Orbifolds.size();
  for (unsigned i = 0; i < s1; ++i)
  {
    if (OrbifoldLabel == this->Orbifolds[i].OrbifoldGroup.Label)
    {
      if (this->print_output && PrintOutput)
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold labeled \"" << OrbifoldLabel << "\" already exists." << this->Print.cend << endl;
      return true;
    }
  }
  return false;
}



/* ########################################################################################
######   MessageOrbifoldNotKnown(const string &OrbifoldLabel, unsigned &index) const ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) OrbifoldLabel : label of an orbifold be checked                          ######
######   output:                                                                     ######
######   2) index         : the index of "OrbifoldLabel" in the member variable      ######
######                      "Orbifolds"                                              ######
######   return value     : is the orbifold with label "OrbifoldLabel" unknown?      ######
###########################################################################################
######   description:                                                                ######
######   Finds the index of the orbifold "OrbifoldLabel" in the member variable      ######
######   "Orbifolds". If orbifold not found, display a message.                      ######
######################################################################################## */
bool CPrompt::MessageOrbifoldNotKnown(const string &OrbifoldLabel, unsigned &index) const
{
  size_t s1 = this->Orbifolds.size();
  for (unsigned i = 0; i < s1; ++i)
  {
    if (OrbifoldLabel == this->Orbifolds[i].OrbifoldGroup.Label)
    {
      index = i;
      return false;
    }
  }
  if (this->print_output)
    (*this->Print.out) << "\n  " << this->Print.cbegin << "Orbifold \"" << OrbifoldLabel << "\" not known." << this->Print.cend << "\n" << endl;
  return true;
}



/* ########################################################################################
######   MessageParameterNotKnown(const string &parameter_string) const              ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) parameter_string : a parameter given by the user                         ######
######   output:                                                                     ######
######   return value        : is the parameter not empty?                           ######
###########################################################################################
######   description:                                                                ######
######   Display a message if "parameter_string" is not empty.                       ######
######################################################################################## */
bool CPrompt::MessageParameterNotKnown(const string &parameter_string) const
{
  if (parameter_string.find_first_not_of(" ") != string::npos)
  {
    if (this->print_output)
      (*this->Print.out) << "  " << this->Print.cbegin << "bash: " << parameter_string << ": parameters not known." << this->Print.cend << "\n" << endl;
    return true;
  }
  return false;
}



/* ########################################################################################
######   MessageVEVConfigAlreadyExists(const vector<SConfig> &VEVConfigs, ...) const ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) VEVConfigs      : search for "VEVConfigLabel" in here                    ######
######   2) VEVConfigLabel  : 1st part of the label of vev-config to be checked      ######
######   3) VEVConfigNumber : 2nd part of the label of vev-config to be checked      ######
######   output:                                                                     ######
######   return value       : does a vev-config with given label already exist?      ######
###########################################################################################
######   description:                                                                ######
######   Checks whether the given vev-config label already exists.                   ######
######################################################################################## */
bool CPrompt::MessageVEVConfigAlreadyExists(const vector<SConfig> &VEVConfigs, const string &VEVConfigLabel, const unsigned &VEVConfigNumber) const
{
  const size_t s1 = VEVConfigs.size();
  for (unsigned i = 0; i < s1; ++i)
  {
    if ((VEVConfigs[i].ConfigLabel == VEVConfigLabel) && (VEVConfigs[i].ConfigNumber == VEVConfigNumber))
    {
      if (this->print_output)
        (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"" << VEVConfigLabel << VEVConfigNumber << "\" already exists." << this->Print.cend << endl;
      return true;
    }
  }
  return false;
}



/* ########################################################################################
######   MessageVEVConfigNotKnown(const vector<SConfig> &VEVConfigs, ...) const      ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) VEVConfigs      : search for "VEVConfigLabel" in here                    ######
######   2) VEVConfigLabel  : 1st part of the label of a vev-config be checked       ######
######   3) VEVConfigNumber : 2nd part of the label of a vev-config be checked       ######
######   output:                                                                     ######
######   4) index           : the index of the vev-config label in "VEVConfigs"      ######
######   return value       : is the vev-config label unknown?                       ######
###########################################################################################
######   description:                                                                ######
######   Finds the index of the vev-config "VEVConfigLabel""VEVConfigNumber" in      ######
######   "VEVConfigs". If vev-config not found, display a message.                   ######
######################################################################################## */
bool CPrompt::MessageVEVConfigNotKnown(const vector<SConfig> &VEVConfigs, const string &VEVConfigLabel, const unsigned &VEVConfigNumber, unsigned &index) const
{
  string tmp = "";
  const size_t s1 = VEVConfigs.size();
  for (unsigned i = 0; i < s1; ++i)
  {
    if ((VEVConfigs[i].ConfigLabel == VEVConfigLabel) && (VEVConfigs[i].ConfigNumber == VEVConfigNumber))
    {
      index = i;
      return false;
    }
  }
  if (this->print_output)
    (*this->Print.out) << "\n  " << this->Print.cbegin << "Vev-configuration \"" << VEVConfigLabel << VEVConfigNumber << "\" not known." << this->Print.cend << "\n" << endl;
  return true;
}



/* ########################################################################################
######   MessageVEVConfigAlreadyExists(const vector<SConfig> &VEVConfigs, ...) const ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) NamesOfSetsOfX  : search for "X_Label" in here                           ######
######   2) X               : the type of label, e.g. "Set" or "Monomial"            ######
######   3) X_Label         : label to be checked                                    ######
######   output:                                                                     ######
######   return value       : does NamesOfSetsOfX contain "X_Label"?                 ######
###########################################################################################
######   description:                                                                ######
######   Checks whether the given vev-config label already exists.                   ######
######################################################################################## */
bool CPrompt::MessageXAlreadyExists(const vector<string> &NamesOfSetsOfX, const string &X, const string &X_Label) const
{
  if (find(NamesOfSetsOfX.begin(), NamesOfSetsOfX.end(), X_Label) != NamesOfSetsOfX.end())
  {
    if (this->print_output)
      (*this->Print.out) << "\n  " << this->Print.cbegin << X << " \"" << X_Label << "\" already exists." << this->Print.cend << endl;
    return true;
  }
  return false;
}



/* ########################################################################################
######   MessageXNotKnown(const vector<string> &NamesOfSetsOfX, ...) const           ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) NamesOfSetsOfX : search for "X_Label" in this set                        ######
######   2) X              : the type of label, e.g. "Set" or "Monomial"             ######
######   3) X_Label        : label to search for                                     ######
######   output:                                                                     ######
######   4) index          : the index of "X_Label" in "NamesOfSetsOfX"              ######
######   return value      : is the label unknown?                                   ######
###########################################################################################
######   description:                                                                ######
######   Finds the index of "X_Label" in "NamesOfSetsOfX". If not found, display a   ######
######   message.                                                                    ######
######################################################################################## */
bool CPrompt::MessageXNotKnown(const vector<string> &NamesOfSetsOfX, const string &X, const string &X_Label, unsigned &index) const
{
  vector<string>::const_iterator pos = find(NamesOfSetsOfX.begin(), NamesOfSetsOfX.end(), X_Label);
  if (pos == NamesOfSetsOfX.end())
  {
    if (this->print_output)
      (*this->Print.out) << "\n  " << this->Print.cbegin << X << " \"" << X_Label << "\" not known." << this->Print.cend << "\n" << endl;
    return true;
  }
  index = distance(NamesOfSetsOfX.begin(), pos);
  return false;
}



/* ########################################################################################
######   PrintCurrentDirectory(string &output) const                                 ######
######                                                                               ######
######   Version: 10.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   -                                                                           ######
######   output:                                                                     ######
######   1) output    : write current directory string here                          ######
######   return value : is current directory valid?                                  ######
######################################################################################## */
bool CPrompt::PrintCurrentDirectory(string &output) const
{
  if (this->current_folder[0] == -1)
  {
    output = "/> ";
    return true;
  }
  else
  {
    output = "/" + this->Orbifolds[this->OrbifoldIndex].OrbifoldGroup.Label;
    switch (this->current_folder[1])
    {
      case 0:
      {
        output += "> ";
        return true;
      }
      case 1:
      {
        output += "/model> ";
        return true;
      }
      case 2:
      {
        output += "/gauge group> ";
        return true;
      }
      case 3:
      {
        output += "/spectrum> ";
        return true;
      }
      case 4:
      {
        output += "/couplings> ";
        return true;
      }
      case 5:
      {
        output += "/vev-config";
        switch (this->current_folder[2])
        {
          case 0:
          {
            output += "> ";
            return true;
          }
          case 1:
          {
            output += "/labels> ";
            return true;
          }
        }
      }
    }
  }
  return false;
}



/* ########################################################################################
######   PrintCommandsConditions() const                                             ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   -                                                                           ######
######   output:                                                                     ######
######   -                                                                           ######
######################################################################################## */
void CPrompt::PrintCommandsConditions() const
{
  if (!this->print_output)
    return;

  (*this->Print.out) << "\n  if(condition):\n";
  (*this->Print.out) << "    A condition consists of three parts, e.g. if(length == even):\n\n";
  (*this->Print.out) << "      1) left hand side: the variable:\n";
  (*this->Print.out) << "        \"length\"  : the length-square of the left-moving momentum, (p_sh)^2\n";
  (*this->Print.out) << "        \"vev\"     : the vacuum expectation value\n";
  (*this->Print.out) << "        \"B-L\"     : the B-L charge\n";
  (*this->Print.out) << "        \"Q_i\"     : i-th U(1) charge, i = 1,2,3,...\n";
  (*this->Print.out) << "        \"acc. Q_i\": i-th accidental U(1) charge, i = 1,2,3,...\n";
  (*this->Print.out) << "        \"p_sh_i\"  : i-th component of the left-moving momentum p_sh, i = 1,..,16\n";
  (*this->Print.out) << "        \"q_sh_i\"  : i-th component of the right-moving momentum q_sh, i = 1,..,4\n";
  (*this->Print.out) << "        \"#osci.\"  : number of oscillators acting on the left-mover\n";
  (*this->Print.out) << "        \"label\"   : will compare the field label\n\n";
  (*this->Print.out) << "      2) in between: the comparison operator\n";
  (*this->Print.out) << "        \"==\", \"!=\", \">\", \">=\", \"<\" and \"<=\" or \n";
  (*this->Print.out) << "        \"involves\", \"!involves\" for field labels\n";
  (*this->Print.out) << "      3) right hand side: the value\n";
  (*this->Print.out) << "        \"even\"    : only with comparison \"==\" and \"!=\"\n";
  (*this->Print.out) << "        \"odd\"     : only with comparison \"==\" and \"!=\"\n";
  (*this->Print.out) << "        \"even/odd\": only with comparison \"==\" and \"!=\"\n";
  (*this->Print.out) << "        string    : check whether the field labels involve \"string\" or not\n";
  (*this->Print.out) << "        rational  : a rational number, e.g. \"0\" and \"1/2\"\n\n";
  (*this->Print.out) << "    More examples:\n";
  (*this->Print.out) << "      1) if(length == 3/2)\n";
  (*this->Print.out) << "      2) if(vev != 0)\n";
  (*this->Print.out) << "      3) if(B-L == even)\n";
  (*this->Print.out) << "      4) if(Q_1 > 1/3)\n";
  (*this->Print.out) << "      5) if(#osci. != 0)\n";
  (*this->Print.out) << "      5) if(label involves X)\n\n" << flush;
}



/* ########################################################################################
######   PrintCommandsMonomials() const                                              ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   -                                                                           ######
######   output:                                                                     ######
######   -                                                                           ######
######################################################################################## */
void CPrompt::PrintCommandsMonomials() const
{
  if (!this->print_output)
    return;

  (*this->Print.out) << "\n  gauge invariant monomials:\n";
  (*this->Print.out) << "    create monomial(MonomialLabel)\n";
  (*this->Print.out) << "    delete monomial(MonomialLabel)\n";
  (*this->Print.out) << "    delete monomials\n";
  (*this->Print.out) << "    insert(fields) into monomial(MonomialLabel)\n";
  (*this->Print.out) << "    print monomial(MonomialLabel)\n";
  (*this->Print.out) << "    print monomials                           optional: \"check Q_anom quantization\"\n";
  (*this->Print.out) << "                                                        \"cancelling FI term\"\n\n" << flush;
}



/* ########################################################################################
######   PrintCommandsProcesses() const                                              ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   -                                                                           ######
######   output:                                                                     ######
######   -                                                                           ######
######################################################################################## */
void CPrompt::PrintCommandsProcesses() const
{
  if (!this->print_output)
    return;

  (*this->Print.out) << "\n  processes:\n";
  (*this->Print.out) << "    ps                                        list all active processes\n";
  (*this->Print.out) << "    kill(A)                                   terminate process with ID \"A\"; use \"kill(all)\" to terminate all\n";
  (*this->Print.out) << "    wait(X)                                   wait until all processes have been terminated (check every \"X\" seconds)\n\n" << flush;
}



/* ########################################################################################
######   PrintCommandsSets() const                                                   ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   -                                                                           ######
######   output:                                                                     ######
######   -                                                                           ######
######################################################################################## */
void CPrompt::PrintCommandsSets() const
{
  if (!this->print_output)
    return;

  (*this->Print.out) << "\n  sets of fields:\n";
  (*this->Print.out) << "    create set(SetLabel)                      optional: \"from monomials\"\n";
  (*this->Print.out) << "                                                        \"from monomial(MonomialLabel)\"\n";
  (*this->Print.out) << "    delete set(SetLabel)\n";
  (*this->Print.out) << "    delete sets\n";
  (*this->Print.out) << "    insert(fields) into set(SetLabel)         optional: \"if(condition)\"\n";
  (*this->Print.out) << "    remove(fields) from set(SetLabel)         optional: \"if(condition)\"\n";
  (*this->Print.out) << "    print sets                                optional: \"if not empty\"\n";
  (*this->Print.out) << "    print set(SetLabel)\n";
  (*this->Print.out) << "    #fields in set(SetLabel)\n\n" << flush;
}



/* ########################################################################################
######   PrintFor(unsigned number_of_Type, const string &Type, ...) const            ######
######                                                                               ######
######   Version: 10.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) number_of_Type : max number for the index "Var"                          ######
######   2) Type           : what the index runs through, e.g. "space group"         ######
######   3) Var            : the index label, e.g. "i"                               ######
######   output:                                                                     ######
######   -                                                                           ######
######################################################################################## */
void CPrompt::PrintFor(unsigned number_of_Type, const string &Type, const string &Var) const
{
  switch (number_of_Type)
  {
    case 0:
      (*this->Print.out) << "Warning: no " << Type << " available!\n";
      break;
    case 1:
      (*this->Print.out) << "for " << Var << " = 1\n";
      break;
    case 2:
      (*this->Print.out) << "for " << Var << " = 1,2\n";
      break;
    case 3:
      (*this->Print.out) << "for " << Var << " = 1,2,3\n";
      break;
    default:
      (*this->Print.out) << "for " << Var << " = 1,...," << number_of_Type << "\n";
  }
}



/* ########################################################################################
######   FindSpaceGroupsInDirectory(const unsigned &M, const unsigned &N, ...)       ######
######                                                                               ######
######   Version: 29.06.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) M         : order of Z_M                                                 ######
######   2) N         : order of Z_N                                                 ######
######   3) directory : directory of local computer where the geometry files are     ######
######                  stored, default should be "Geometry/"                        ######
######   output:                                                                     ######
######   return value : finished succesfully?                                        ######
######################################################################################## */
/*bool CPrompt::FindSpaceGroupsInDirectory(const unsigned &M, const unsigned &N, const string &directory)
{
  this->PV.AvailableLatticesFilenames.clear();
  this->PV.AvailableLatticesLabels.clear();
  this->PV.AvailableAdditionalLabels.clear();

  std::ostringstream os1, os2;
  os1 << M;

  string tmp = "dir " + directory + "Geometry_Z" + os1.str();
  if (N != 1)
  {
    os2 << N;
    tmp += "xZ" + os2.str();
  }
  tmp += "*.txt";

  // begin: search in the directory "Geometry" for possible files
  vector<string> PossibleFilenames;

  string::size_type loc1 = 0;
  string::size_type loc2 = 0;
  char line[130];
  FILE *fptr = popen(tmp.c_str(), "r");
  while (fgets( line, sizeof line, fptr))
  {
    tmp = line;

    loc1 = 0;
    loc2 = 0;
    while (loc1 != string::npos)
    {
      loc1 = tmp.find("Geometry_", loc2);
      if (loc1 != string::npos)
      {
        tmp = tmp.substr(loc1, string::npos);
        loc2 = tmp.find(".txt", 0);
        if (loc2 != string::npos)
        {
          PossibleFilenames.push_back(directory + tmp.substr(0, loc2+4));
          loc1 = loc2 + 1;
        }
      }
    }
  }
  pclose(fptr);
  // end: search in the directory "Geometry" for possible files

  string tmp_additional_label = "";
  string tmp_lattice_label    = "";

  bool go_on = true;
  bool PointGroupFound = false;

  const size_t s0 = PossibleFilenames.size();
  for (unsigned i = 0; i < s0; ++i)
  {
    std::ifstream in;
    in.open(PossibleFilenames[i].data(), ifstream::in);
    if(!in.is_open() || !in.good())
    {
      cout << "\n  Warning in bool CPrompt::FindSpaceGroupsInDirectory(...) : Could not find the file \"" << PossibleFilenames[i] << "\". Return false." << endl;
      return false;
    }

    // begin: find if current file fits to the chosen orbifold and get the lattice_label
    tmp_additional_label = "";
    tmp_lattice_label    = "";

    go_on = true;
    PointGroupFound = false;

    while (go_on && (!PointGroupFound || (tmp_additional_label == "") || (tmp_lattice_label == "")) && GetSaveLine(in, tmp))
    {
      if (tmp.substr(0,11) == "point group")
      {
        unsigned tmp_M = 0;
        unsigned tmp_N = 0;

        GetSaveLine(in, tmp);
        std::istringstream line1(tmp);
        line1 >> tmp_M;

        GetSaveLine(in, tmp);
        std::istringstream line2(tmp);
        line2 >> tmp_N;

        PointGroupFound = true;
        if ((tmp_M != M) || (tmp_N != N))
          go_on = false;
      }
      else
        if (tmp.substr(0,16) == "additional label")
          GetSaveLine(in, tmp_additional_label);
      else
        if (tmp.substr(0,13) == "lattice label")
          GetSaveLine(in, tmp_lattice_label);
    }
    in.close();
    // end: find if current file fits to the chosen orbifold and get the lattice_label

    if (!PointGroupFound || (tmp_lattice_label == ""))
      go_on = false;

    if (go_on)
    {
      this->PV.AvailableLatticesFilenames.push_back(PossibleFilenames[i]);
      this->PV.AvailableLatticesLabels.push_back(tmp_lattice_label);
      this->PV.AvailableAdditionalLabels.push_back(tmp_additional_label);
    }
  }
  return true;
}
*/
////////////////////////////////////////////////////////////
// J28 begin mod block FindSpaceGroupsInDirectory
bool CPrompt::FindSpaceGroupsInDirectory(const unsigned &N, const unsigned &K, const string &directory)
{
  this->PV.AvailableLatticesFilenames.clear();
  this->PV.AvailableLatticesLabels.clear();
  this->PV.AvailableAdditionalLabels.clear();

  std::ostringstream os1, os2;
  os1 << N;

  string tmp = "dir " + directory + "Geometry_Z" + os1.str();
  if (K != 1)
  {
    os2 << K;
    tmp += "xZ" + os2.str();
  }
  tmp += "*.txt";

  // begin: search in the directory "Geometry" for possible files
  vector<string> PossibleFilenames;

  string::size_type loc1 = 0;
  string::size_type loc2 = 0;
  char line[130];
  FILE *fptr = popen(tmp.c_str(), "r");
  while (fgets( line, sizeof line, fptr))
  {
    tmp = line;

    loc1 = 0;
    loc2 = 0;
    while (loc1 != string::npos)
    {
      loc1 = tmp.find("Geometry_", loc2);
      if (loc1 != string::npos)
      {
        tmp = tmp.substr(loc1, string::npos);
        loc2 = tmp.find(".txt", 0);
        if (loc2 != string::npos)
        {
          PossibleFilenames.push_back(directory + tmp.substr(0, loc2+4));
          loc1 = loc2 + 1;
        }
      }
    }
  }
  pclose(fptr);
  // end: search in the directory "Geometry" for possible files

  string tmp_additional_label = "";
  string tmp_lattice_label    = "";

  bool go_on = true;
  bool PointGroupFound = false;

  const size_t s0 = PossibleFilenames.size();
  for (unsigned i = 0; i < s0; ++i)
  {
    std::ifstream in;
    in.open(PossibleFilenames[i].data(), ifstream::in);
    if(!in.is_open() || !in.good())
    {
      cout << "\n  Warning in bool CPrompt::FindSpaceGroupsInDirectory(...) : Could not find the file \"" << PossibleFilenames[i] << "\". Return false." << endl;
      return false;
    }

    // begin: find if current file fits to the chosen orbifold and get the lattice_label
/*    tmp_additional_label = "";
    tmp_lattice_label    = "";

    go_on = true;
    PointGroupFound = false;

    while (go_on && (!PointGroupFound || (tmp_additional_label == "") || (tmp_lattice_label == "")) && GetSaveLine(in, tmp))
    {
      if (tmp.substr(0,11) == "point group")
      {
        unsigned tmp_M = 0;
        unsigned tmp_N = 0;

        GetSaveLine(in, tmp);
        std::istringstream line1(tmp);
        line1 >> tmp_M;

        GetSaveLine(in, tmp);
        std::istringstream line2(tmp);
        line2 >> tmp_N;

        PointGroupFound = true;
        if ((tmp_M != M) || (tmp_N != N))
          go_on = false;
      }
      else
        if (tmp.substr(0,16) == "additional label")
          GetSaveLine(in, tmp_additional_label);
      else
        if (tmp.substr(0,13) == "lattice label")
          GetSaveLine(in, tmp_lattice_label);
    }
    in.close();
*/
    // end: find if current file fits to the chosen orbifold and get the lattice_label



///////////////////// J28 begin mod : find if current file ....
    // begin: find if current file fits to the chosen orbifold and get the lattice_label
    tmp_additional_label = "";
    tmp_lattice_label    = "";

    go_on = true;
    PointGroupFound = false;

    while (go_on && (!PointGroupFound || (tmp_additional_label == "") || (tmp_lattice_label == "")) && GetSaveLine(in, tmp))
    {
      if (tmp.substr(0,11) == "point group")
      {
        unsigned tmp_M = 0;
        unsigned tmp_N = 0;
        unsigned tmp_K = 0; // added J28

        GetSaveLine(in, tmp);
        std::istringstream line1(tmp);
        line1 >> tmp_M;

        GetSaveLine(in, tmp);
        std::istringstream line2(tmp);
        line2 >> tmp_N;

        GetSaveLine(in, tmp);   //added J28, block 3 lines
        std::istringstream line3(tmp);
        line3 >> tmp_K;

        /*PointGroupFound = true;  //orig
        if ((tmp_M != M) || (tmp_N != N))
          go_on = false;*/

         PointGroupFound = true;   //added
        if ((tmp_N != N) || (tmp_K != K))
          go_on = false;


      }
      else
        if (tmp.substr(0,16) == "additional label")
          GetSaveLine(in, tmp_additional_label);
      else
        if (tmp.substr(0,13) == "lattice label")
          GetSaveLine(in, tmp_lattice_label);
    }
    in.close();
    // end: find if current file fits to the chosen orbifold and get the lattice_label
///////////////////// J28 end mod : find if current file ....



    if (!PointGroupFound || (tmp_lattice_label == ""))
      go_on = false;

    if (go_on)
    {
      this->PV.AvailableLatticesFilenames.push_back(PossibleFilenames[i]);
      this->PV.AvailableLatticesLabels.push_back(tmp_lattice_label);
      this->PV.AvailableAdditionalLabels.push_back(tmp_additional_label);
    }
  }
    return true;
}

// J28 end mod block FindSpaceGroupsInDirectory 
/////////////////////////////////////////////////


/* ########################################################################################
######   SplitVEVConfigLabel(string &VEVConfigLabel, unsigned &VEVConfigNumber) const######
######                                                                               ######
######   Version: 29.06.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) VEVConfigLabel  : the full vev-config label, e.g. "Test1"                ######
######   output:                                                                     ######
######   1) VEVConfigLabel  : the label, e.g. "Test"                                 ######
######   2) VEVConfigNumber : the number, e.g. 1                                     ######
######   return value : finished succesfully?                                        ######
######################################################################################## */
bool CPrompt::SplitVEVConfigLabel(string &VEVConfigLabel, unsigned &VEVConfigNumber) const
{
  if (VEVConfigLabel.size() == 0)
  {
    if (this->print_output)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "Empty string is not a valid configuration label. Correct labels look like \"ExampleVEVConfig12\"." << this->Print.cend << endl;
    VEVConfigLabel  = "ErrorLabel";
    VEVConfigNumber = 1;
    return false;
  }

  const size_t pos = VEVConfigLabel.find_first_of("0123456789", 0);
  if (pos == string::npos)
  {
    VEVConfigNumber = 1;
    return true;
  }

  if ((pos != string::npos) && (VEVConfigLabel.find_first_not_of("0123456789", pos) != string::npos))
  {
    if (this->print_output)
      (*this->Print.out) << "\n  " << this->Print.cbegin << "\"" << VEVConfigLabel << "\" is not a valid configuration label. Correct labels look like \"ExampleVEVConfig12\"." << this->Print.cend << endl;
    VEVConfigLabel  = "ErrorLabel";
    VEVConfigNumber = 1;
    return false;
  }

  VEVConfigNumber = atoi((VEVConfigLabel.substr(pos)).c_str());
  VEVConfigLabel  = VEVConfigLabel.substr(0, pos);
  return true;
}



/* ########################################################################################
######   GetIndices(const vector<string> &FieldLabels) const                         ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) FieldLabels : some field labels, e.g. "N Q_1 Q_2"                        ######
######   output:                                                                     ######
######   return value   : the corresponding indices, e.g. the indices of all fields  ######
######                    "N" and the indices of "Q_1" and "Q_2"                     ######
######################################################################################## */
vector<unsigned> CPrompt::GetIndices(const vector<string> &FieldLabels) const
{
  const SConfig &VEVConfig = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]];

  return global_GetIndices(FieldLabels, VEVConfig.use_Labels, VEVConfig.Fields);
}



/* ########################################################################################
######   GetIndicesOnlyFieldWithNumber(const vector<string> &FieldLabels) const      ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) FieldLabels : some field labels, e.g. "Q_1 Q_2"                          ######
######   output:                                                                     ######
######   return value   : the corresponding indices, e.g. the indices of "Q_1" and   ######
######                    "Q_2"                                                      ######
######################################################################################## */
vector<unsigned> CPrompt::GetIndicesOnlyFieldWithNumber(const vector<string> &FieldLabels) const
{
  const SConfig &VEVConfig = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]];

  return global_GetIndicesOnlyFieldWithNumber(FieldLabels, VEVConfig.use_Labels, VEVConfig.Fields);
}



/* ########################################################################################
######   GetLocalization(const string &Localization, CSpaceGroupElement &...) const  ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Localization : string                                                    ######
######   output:                                                                     ######
######   2) result       : the space-group element of the fixed point specified by   ######
######                     the string "Localization"                                 ######
######   return value    : localization found?                                       ######
######################################################################################## */
/*
bool CPrompt::GetLocalization(const string &Localization, CSpaceGroupElement &result) const
{
  if (Localization.substr(0,7) == "loc of ")
  {
    vector<string> FieldLabels;
  FieldLabels.push_back(Localization.substr(7, string::npos));
     
    vector<unsigned> FieldIndices = GetIndices(FieldLabels);

    if (FieldIndices.size() == 1)
    {
      result = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]].Fields[FieldIndices[0]].SGElement;
      return true;
    }
  }
  else
  {
    vector<int> tmp_result;
    convert_string_to_vector_of_int(Localization, tmp_result);

    //if (tmp_result.size() == 8)   //origN1
    
      if (tmp_result.size() == 9)    //trialN0
    {
      //result.Set_k(tmp_result[0]);  //origN1
      //result.Set_l(tmp_result[1]);  //origN1
      
      result.Set_m(tmp_result[0]);  //trialN0
      result.Set_n(tmp_result[1]);  //trialN0
      result.Set_k(tmp_result[2]);   //trialN0
      
      
      result.Set_n_alpha(0, tmp_result[2]);
      result.Set_n_alpha(1, tmp_result[3]);
      result.Set_n_alpha(2, tmp_result[4]);
      result.Set_n_alpha(3, tmp_result[5]);
      result.Set_n_alpha(4, tmp_result[6]);
      result.Set_n_alpha(5, tmp_result[7]);
      return true;
    }
    else
    {
     // if (tmp_result.size() == 2) //origN1
       
       if (tmp_result.size() == 3)  //trialN0
      
      {
    //    result.Set_k(tmp_result[0]);  //origN1
    //    result.Set_l(tmp_result[1]);   //origN1
        
        result.Set_m(tmp_result[0]); //trialN0
        result.Set_n(tmp_result[1]);  //trialN0
        result.Set_k(tmp_result[2]);  //trialN0
        
        result.Set_n_alpha(0, 0);
        result.Set_n_alpha(1, 0);
        result.Set_n_alpha(2, 0);
        result.Set_n_alpha(3, 0);
        result.Set_n_alpha(4, 0);
        result.Set_n_alpha(5, 0);
        return true;
      }
    }
  }
  if (this->print_output)
    (*this->Print.out) << "\n  " << this->Print.cbegin << "Localization \"" << Localization << "\" not found." << this->Print.cend << endl;
  return false;
}
*/

//sept26
/* ########################################################################################
######   GetLocalization(const string &Localization, CSpaceGroupElement &...) const  ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Localization : string                                                    ######
######   output:                                                                     ######
######   2) result       : the space-group element of the fixed point specified by   ######
######                     the string "Localization"                                 ######
######   return value    : localization found?                                       ######
######################################################################################## */
bool CPrompt::GetLocalization(const string &Localization, CSpaceGroupElement &result) const
{
  if (Localization.substr(0,8) == "loc of ")
  {
    vector<string> FieldLabels;
    FieldLabels.push_back(Localization.substr(8, string::npos));
    vector<unsigned> FieldIndices = GetIndices(FieldLabels);

    if (FieldIndices.size() == 1)
    {
      result = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]].Fields[FieldIndices[0]].SGElement;
      return true;
    }
  }
  else
  {
    vector<int> tmp_result;
    convert_string_to_vector_of_int(Localization, tmp_result);

    if (tmp_result.size() == 9)
    {
      result.Set_m(tmp_result[0]);
      result.Set_n(tmp_result[1]);
      result.Set_k(tmp_result[2]);
      result.Set_n_alpha(0, tmp_result[3]);
      result.Set_n_alpha(1, tmp_result[4]);
      result.Set_n_alpha(2, tmp_result[5]);
      result.Set_n_alpha(3, tmp_result[6]);
      result.Set_n_alpha(4, tmp_result[7]);
      result.Set_n_alpha(5, tmp_result[8]);
      return true;
    }
    else
    {
      if (tmp_result.size() == 3)
      {
        result.Set_m(tmp_result[0]);
        result.Set_n(tmp_result[1]);
        result.Set_k(tmp_result[2]);
        result.Set_n_alpha(0, 0);
        result.Set_n_alpha(1, 0);
        result.Set_n_alpha(2, 0);
        result.Set_n_alpha(3, 0);
        result.Set_n_alpha(4, 0);
        result.Set_n_alpha(5, 0);
        return true;
      }
    }
  }
  if (this->print_output)
    (*this->Print.out) << "\n  " << this->Print.cbegin << "Localization \"" << Localization << "\" not found." << this->Print.cend << endl;
  return false;
}




/* ########################################################################################
######   GetLocalization(const string &Localization, CSpaceGroupElement &...) const  ######
######                                                                               ######
######   Version: 19.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) input           : a string either containing a fixed point / brane label ######
######                        or a constructing element                              ######
######   2) Orbifold        : the orbifold to look for "input"                       ######
######   output:                                                                     ######
######   3) FixedBraneFound : fixed point / brane "input" found in "Orbifold"?       ######
######   return value       : access to the fixed point / brane "input"              ######
######################################################################################## */
CFixedBrane &CPrompt::AccessFixedBrane(const string &input, COrbifold &Orbifold, bool &FixedBraneFound) const
{
  unsigned j = 0;

  size_t t1 = Orbifold.GetNumberOfSectors();
  size_t t2 = 0;

  // print summary of fixed point using the fixed point label
  for (unsigned i = 0; i < t1; ++i)
  {
    CSector &Sector = Orbifold.AccessSector(i);
    t2 = Sector.GetNumberOfFixedBranes();

    for (j = 0; j < t2; ++j)
    {
      CFixedBrane &FixedBrane = Sector.AccessFixedBrane(j);
      if (FixedBrane.GetFixedBraneLabel() == input)
      {
        FixedBraneFound = true;
        return FixedBrane;
      }
    }
  }

  // print summary of fixed point using the constructing element
  CSpaceGroupElement SGElement;

  if (this->GetLocalization(input, SGElement))
  {
    FixedBraneFound = true;
    return Orbifold.AccessFixedBrane(SGElement, FixedBraneFound);
  }
  FixedBraneFound = false;
  return Orbifold.AccessSector(0).AccessFixedBrane(0);
}


/* ########################################################################################
######   ExtractLabels(const SUSYMultiplet &Multiplet, string input, ...)            ######
######                                                                               ######
######   Version: 28.03.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Multiplet             : LeftChiral, RightChiral, Vector, ...             ######
######   2) input                 : string                                           ######
######                                                                               ######
######   output:                                                                     ######
######   2) FieldLabels           : vector of field labels                           ######
######                                                                               ######
######################################################################################## */
/*void CPrompt::ExtractLabels(const SUSYMultiplet &Multiplet, string input, vector<string> &FieldLabels)
{
  // begin: remove VEV symbols "<" and ">" from the input string
  string::size_type loc1 = 0;
  while (loc1 != string::npos)
  {
    loc1 = input.find("<", 0);
    if (loc1 != string::npos)
      input.erase(loc1,1);

    loc1 = input.find(">", 0);
    if (loc1 != string::npos)
      input.erase(loc1,1);
  }
  // end: remove VEV symbols "<" and ">" from the input string

  const SConfig &VEVConfig = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]];

  unsigned i = 0;

  string tmp = "";
  string tmp_string1 = "";
  string tmp_string2 = "";
  loc1 = 0;
  string::size_type loc2 = 0;
  string::size_type loc3 = 0;
  size_t t1 = 0;

  vector<string>::const_iterator pos;

  // run through the string
  while (loc1 != string::npos)
  {
    loc1 = input.find(" ", loc2);

    // extract label
    tmp_string1 = input.substr(loc2, loc1 - loc2);
    loc2 = loc1 + 1;
    //cout << "Label->" << tmp_string1 << "<-" << endl;

    // begin: find operators
    loc3  = tmp_string1.find_last_of("+-\\");
    if (loc3 != string::npos)
    {
      vector<string> FieldLabelsA;
      vector<string> FieldLabelsB;
      ExtractLabels(Multiplet, tmp_string1.substr(0, loc3),              FieldLabelsA);
      ExtractLabels(Multiplet, tmp_string1.substr(loc3+1, string::npos), FieldLabelsB);

      string op = tmp_string1.substr(loc3, 1);
      // begin: find difference A\B or A-B
      if ((op.substr(0,1) == "-") || (op.substr(0,1) == "\\"))
      {
        //cout << "minus" << endl;
        t1 = FieldLabelsA.size();
        for (i = 0; i < t1; ++i)
        {
          tmp = FieldLabelsA[i];
          // add label only if not contained before and if label not contained in set(B)
          if ((find(FieldLabelsB.begin(), FieldLabelsB.end(), tmp) == FieldLabelsB.end()) && (find(FieldLabels.begin(), FieldLabels.end(), tmp) == FieldLabels.end()))
            FieldLabels.push_back(tmp);
        }
      }
      // end: find difference A\B or A-B
      else
      // begin: find union A+B
      if (op.substr(0,1) == "+")
      {
        //cout << "plus" << endl;
        t1 = FieldLabelsA.size();
        for (i = 0; i < t1; ++i)
        {
          tmp = FieldLabelsA[i];
          // add label only if not contained before
          if (find(FieldLabels.begin(), FieldLabels.end(), tmp) == FieldLabels.end())
            FieldLabels.push_back(tmp);
        }
        t1 = FieldLabelsB.size();
        for (i = 0; i < t1; ++i)
        {
          tmp = FieldLabelsB[i];
          // add label only if not contained before
          if (find(FieldLabels.begin(), FieldLabels.end(), tmp) == FieldLabels.end())
            FieldLabels.push_back(tmp);
        }
      }
      // end: find union A+B
    }
    // end: find operators
    else
    {
      //cout << "no operator" << endl;
      // begin: insert all fields
      if (tmp_string1 == "*")
      {
        t1 = VEVConfig.Fields.size();
        for (i = 0; i < t1; ++i)
        {
          const CField &Field = VEVConfig.Fields[i];

          if ((Field.Multiplet == Multiplet) || (Multiplet == AnyKind))
          {
            tmp_string2 = Field.Labels[VEVConfig.use_Labels];
            std::ostringstream os;
            os << Field.Numbers[VEVConfig.use_Labels];
            tmp_string2 += "_";
            tmp_string2 += os.str();

            // add label only if not contained before
            if (find(FieldLabels.begin(), FieldLabels.end(), tmp_string2) == FieldLabels.end())
              FieldLabels.push_back(tmp_string2);
          }
        }
        return;
      }
      // end: insert all fields

      pos = find(VEVConfig.NamesOfSetsOfFields.begin(), VEVConfig.NamesOfSetsOfFields.end(), tmp_string1);
      // begin: "tmp_string1" labels a field or several fields
      if (pos == VEVConfig.NamesOfSetsOfFields.end())
      {
        // a single field
        if (tmp_string1.find("_") != string::npos)
        {
          t1 = VEVConfig.Fields.size();
          for (i = 0; i < t1; ++i)
          {
            const CField &Field = VEVConfig.Fields[i];

            if ((Field.Multiplet == Multiplet) || (Multiplet == AnyKind))
            {
              tmp_string2 = Field.Labels[VEVConfig.use_Labels];
              std::ostringstream os;
              os << Field.Numbers[VEVConfig.use_Labels];
              tmp_string2 += "_";
              tmp_string2 += os.str();

              // add label only if not contained before
              if ((tmp_string2 == tmp_string1) && (find(FieldLabels.begin(), FieldLabels.end(), tmp_string2) == FieldLabels.end()))
                FieldLabels.push_back(tmp_string2);
            }
          }
        }
        // several fields
        else
        {
          t1 = VEVConfig.Fields.size();
          for (i = 0; i < t1; ++i)
          {
            const CField &Field = VEVConfig.Fields[i];

            if ((Field.Multiplet == Multiplet) || (Multiplet == AnyKind))
            {
              tmp_string2 = Field.Labels[VEVConfig.use_Labels];
              std::ostringstream os;
              os << Field.Numbers[VEVConfig.use_Labels];
              tmp_string2 += "_";
              tmp_string2 += os.str();

              // add label only if not contained before
              if ((Field.Labels[VEVConfig.use_Labels] == tmp_string1) && (find(FieldLabels.begin(), FieldLabels.end(), tmp_string2) == FieldLabels.end()))
                FieldLabels.push_back(tmp_string2);
            }
          }
        }
      }
      // end: "tmp_string1" labels a field or several fields
      // begin: "tmp_string1" labels a set
      else
      {
        const vector<unsigned> &CurrentSet = VEVConfig.SetsOfFields[distance(VEVConfig.NamesOfSetsOfFields.begin(), pos)];

        t1 = CurrentSet.size();
        for (i = 0; i < t1; ++i)
        {
          const CField &Field = VEVConfig.Fields[CurrentSet[i]];

          if ((Field.Multiplet == Multiplet) || (Multiplet == AnyKind))
          {
            tmp_string2 = Field.Labels[VEVConfig.use_Labels];
            std::ostringstream os;
            os << Field.Numbers[VEVConfig.use_Labels];
            tmp_string2 += "_";
            tmp_string2 += os.str();

            // add label only if not contained before
            if (find(FieldLabels.begin(), FieldLabels.end(), tmp_string2) == FieldLabels.end())
              FieldLabels.push_back(tmp_string2);
          }
        }
      }
      // end: "tmp_string1" labels a set
    }
  }
}
*/


//Add another one for vector multiplets , June20
// Help: See PrintSummaryOfVEVConfig en cprint.cpp,h, N=1,0.
// MOD v2 de N=1 para adaptar a N=0, Junio20
// ExtractLabels TO MODIFY v2
//Full orig + mods
/* ########################################################################################
######   ExtractLabels(const SUSYMultiplet &Multiplet, string input, ...)            ######
######                                                                               ######
######   Version: 28.03.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Multiplet             : LeftChiral, RightChiral, Vector, ...             ######
######   2) input                 : string                                           ######
######                                                                               ######
######   output:                                                                     ######
######   2) FieldLabels           : vector of field labels                           ######
######                                                                               ######
######################################################################################## */
//begin: july 5, 2020
void CPrompt::ExtractLabels(const vector<SUSYMultiplet>  &Multiplet, string input, vector<string> &FieldLabels)
{
  // begin: remove VEV symbols "<" and ">" from the input string
  string::size_type loc1 = 0;
  while (loc1 != string::npos)
  {
    loc1 = input.find("<", 0);
    if (loc1 != string::npos)
      input.erase(loc1,1);

    loc1 = input.find(">", 0);
    if (loc1 != string::npos)
      input.erase(loc1,1);
  }
  // end: remove VEV symbols "<" and ">" from the input string

  const SConfig &VEVConfig = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]];

  unsigned i = 0;

  string tmp = "";
  string tmp_string1 = "";
  string tmp_string2 = "";
  loc1 = 0;
  string::size_type loc2 = 0;
  string::size_type loc3 = 0;
  size_t t1 = 0;

  vector<string>::const_iterator pos;

  // run through the string
  while (loc1 != string::npos)
  {
    loc1 = input.find(" ", loc2);

    // extract label
    tmp_string1 = input.substr(loc2, loc1 - loc2);
    loc2 = loc1 + 1;
    //cout << "Label->" << tmp_string1 << "<-" << endl;

    // begin: find operators
    loc3  = tmp_string1.find_last_of("+-\\");
    if (loc3 != string::npos)
    {
      vector<string> FieldLabelsA;
      vector<string> FieldLabelsB;
      ExtractLabels(Multiplet, tmp_string1.substr(0, loc3),              FieldLabelsA);
      ExtractLabels(Multiplet, tmp_string1.substr(loc3+1, string::npos), FieldLabelsB);

      string op = tmp_string1.substr(loc3, 1);
      // begin: find difference A\B or A-B
      if ((op.substr(0,1) == "-") || (op.substr(0,1) == "\\"))
      {
        //cout << "minus" << endl;
        t1 = FieldLabelsA.size();
        for (i = 0; i < t1; ++i)
        {
          tmp = FieldLabelsA[i];
          // add label only if not contained before and if label not contained in set(B)
          if ((find(FieldLabelsB.begin(), FieldLabelsB.end(), tmp) == FieldLabelsB.end()) && (find(FieldLabels.begin(), FieldLabels.end(), tmp) == FieldLabels.end()))
            FieldLabels.push_back(tmp);
        }
      }
      // end: find difference A\B or A-B
      else
      // begin: find union A+B
      if (op.substr(0,1) == "+")
      {
        //cout << "plus" << endl;
        t1 = FieldLabelsA.size();
        for (i = 0; i < t1; ++i)
        {
          tmp = FieldLabelsA[i];
          // add label only if not contained before
          if (find(FieldLabels.begin(), FieldLabels.end(), tmp) == FieldLabels.end())
            FieldLabels.push_back(tmp);
        }
        t1 = FieldLabelsB.size();
        for (i = 0; i < t1; ++i)
        {
          tmp = FieldLabelsB[i];
          // add label only if not contained before
          if (find(FieldLabels.begin(), FieldLabels.end(), tmp) == FieldLabels.end())
            FieldLabels.push_back(tmp);
        }
      }
      // end: find union A+B
    }
    // end: find operators
    else
    {
      //cout << "no operator" << endl;
      // begin: insert all fields
      if (tmp_string1 == "*")
      {
      t1 = VEVConfig.Fields.size();
        
        for (i = 0; i < t1; ++i)
        {
			
		 for (int j=0; j<Multiplet.size(); j++)
         { //dp 	
         
         const CField &Field = VEVConfig.Fields[i];
          

//          if ((Field.Multiplet == Multiplet) || (Multiplet == AnyKind))
          if (Field.Multiplet == Multiplet[j])
          {
            tmp_string2 = Field.Labels[VEVConfig.use_Labels];
            std::ostringstream os;
            os << Field.Numbers[VEVConfig.use_Labels];
            tmp_string2 += "_";
            tmp_string2 += os.str();

            // add label only if not contained before
            if (find(FieldLabels.begin(), FieldLabels.end(), tmp_string2) == FieldLabels.end())
              FieldLabels.push_back(tmp_string2);
          }
         } //dp 
        }
        return;
      }
      // end: insert all fields

      pos = find(VEVConfig.NamesOfSetsOfFields.begin(), VEVConfig.NamesOfSetsOfFields.end(), tmp_string1);
      // begin: "tmp_string1" labels a field or several fields
      if (pos == VEVConfig.NamesOfSetsOfFields.end())
      {
        // a single field
        if (tmp_string1.find("_") != string::npos)
        {
          t1 = VEVConfig.Fields.size();
          for (i = 0; i < t1; ++i)
          {
            for (int j=0; j<Multiplet.size(); j++)
            { //dp 	
                
            const CField &Field = VEVConfig.Fields[i];

//            if ((Field.Multiplet == Multiplet) || (Multiplet == AnyKind))
             if (Field.Multiplet == Multiplet[j])
            {
              tmp_string2 = Field.Labels[VEVConfig.use_Labels];
              std::ostringstream os;
              os << Field.Numbers[VEVConfig.use_Labels];
              tmp_string2 += "_";
              tmp_string2 += os.str();

              // add label only if not contained before
              if ((tmp_string2 == tmp_string1) && (find(FieldLabels.begin(), FieldLabels.end(), tmp_string2) == FieldLabels.end()))
                FieldLabels.push_back(tmp_string2);
            }
           }//dp
          }
        }
        // several fields
        else
        {
          t1 = VEVConfig.Fields.size();
          for (i = 0; i < t1; ++i)
          {

            for (int j=0; j<Multiplet.size(); j++)
            { //dp 	

            const CField &Field = VEVConfig.Fields[i];

//            if ((Field.Multiplet == Multiplet) || (Multiplet == AnyKind))
             if (Field.Multiplet == Multiplet[j])
            {
              tmp_string2 = Field.Labels[VEVConfig.use_Labels];
              std::ostringstream os;
              os << Field.Numbers[VEVConfig.use_Labels];
              tmp_string2 += "_";
              tmp_string2 += os.str();

              // add label only if not contained before
              if ((Field.Labels[VEVConfig.use_Labels] == tmp_string1) && (find(FieldLabels.begin(), FieldLabels.end(), tmp_string2) == FieldLabels.end()))
                FieldLabels.push_back(tmp_string2);
            }
           } //dp
          }
        }
      }
      // end: "tmp_string1" labels a field or several fields
      // begin: "tmp_string1" labels a set
      else
      {
        const vector<unsigned> &CurrentSet = VEVConfig.SetsOfFields[distance(VEVConfig.NamesOfSetsOfFields.begin(), pos)];

        t1 = CurrentSet.size();
        for (i = 0; i < t1; ++i)
        {
			
        for (int j=0; j<Multiplet.size(); j++)
         { //dp 

          const CField &Field = VEVConfig.Fields[CurrentSet[i]];

//          if ((Field.Multiplet == Multiplet) || (Multiplet == AnyKind))
           if (Field.Multiplet == Multiplet[j])
          {
            tmp_string2 = Field.Labels[VEVConfig.use_Labels];
            std::ostringstream os;
            os << Field.Numbers[VEVConfig.use_Labels];
            tmp_string2 += "_";
            tmp_string2 += os.str();

            // add label only if not contained before
            if (find(FieldLabels.begin(), FieldLabels.end(), tmp_string2) == FieldLabels.end())
              FieldLabels.push_back(tmp_string2);
          }
         } //dp
        }
      }
      // end: "tmp_string1" labels a set
    }
  }
}
// end: july 5, 2020
       
/* ########################################################################################
######   FindSUSYType(string &input_string, ...) const                               ######
######                                                                               ######
######   Version: 04.07.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) input_string          : input command string                             ######
######   2) NumberOfSupersymmetry : number of SUSY                                   ######
######   output:                                                                     ######
######   3) Multiplet             : the SUSY multiplet type, e.g. "LeftChiral"       ######
######   return value             : did a problem occur?                             ######
###########################################################################################
######   description:                                                                ######
######   Finds SUSY type in "input_string".                                          ######
######################################################################################## */
bool CPrompt::FindSUSYType(string &input_string, int NumberOfSupersymmetry, SUSYMultiplet &Multiplet) const
{
  switch (NumberOfSupersymmetry)
  {
    case 1:
    {
      Multiplet = LeftChiral;
      break;
    }
    case 2:
    {
      Multiplet = AnyKind;
      break;
    }
  }
  
  if (this->FindParameterType1(input_string, "left-chiral"))
  {
    Multiplet = LeftChiral;
    return true;
  }
  if (this->FindParameterType1(input_string, "right-chiral"))
  {
    Multiplet = RightChiral;
    return true;
  }
  if (this->FindParameterType1(input_string, "anykind"))
  {
    Multiplet = AnyKind;
    return true;
  }
  if (this->FindParameterType1(input_string, "vectorcc"))
  {
    Multiplet = VectorCC;
    return true;
  }
  if (this->FindParameterType1(input_string, "vector"))
  {
    Multiplet = Vector;
    return true;
  }
  if (this->FindParameterType1(input_string, "halfhyper"))
  {
    Multiplet = Halfhyper;
    return true;
  }
  if (this->FindParameterType1(input_string, "hyper"))
  {
    Multiplet = Hyper;
    return true;
  }
  if (this->FindParameterType1(input_string, "gravitycc"))
  {
    Multiplet = GravityCC;
    return true;
  }
  if (this->FindParameterType1(input_string, "gravity"))
  {
    Multiplet = Gravity;
    return true;
  }
  if (this->FindParameterType1(input_string, "modulus"))
  {
    Multiplet = LCModulus;
    return true;
  }
  if (this->FindParameterType1(input_string, "moduluscc"))
  {
    Multiplet = RCModulus;
    return true;
  }
  return false;
}



/* ########################################################################################
######   FindConditionsAndFilterFieldIndices(...) const                              ######
######                                                                               ######
######   Version: 26.01.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) input_string : if-conditions are searched in and removed from this       ######
######                     string                                                    ######
######   2) FieldIndices : vector of field indices; the ones that do not fulfill all ######
######                     all conditions will be removed                            ######
######   output:                                                                     ######
######   return value    : did a problem occur?                                      ######
###########################################################################################
######   description:                                                                ######
######   First call "FindConditions(...)" then "ApplyConditions(...)".               ######
######################################################################################## */
bool CPrompt::FindConditionsAndFilterFieldIndices(string &input_string, vector<unsigned> &FieldIndices) const
{
  vector<SCondition> Conditions;
  if (this->FindConditions(input_string, Conditions))
  {
    if (!this->ApplyConditions(Conditions, FieldIndices))
    {
      cout << "Warning in bool CPrompt::FindConditionsAndFilterFieldIndices(...) const: Could not apply the conditions. Return false." << endl;
      return false;
    }
  }
  return true;
}



/* ########################################################################################
######   FindConditions(string &input_string, vector<SCondition> &Conditions) const  ######
######                                                                               ######
######   Version: 16.05.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) input_string : if-conditions are searched in and removed from this       ######
######                     string                                                    ######
######   2) Conditions   : set of conditions extracted from the input string         ######
######   output:                                                                     ######
######   return value    : are conditions found in the input string?                 ######
###########################################################################################
######   description:                                                                ######
######   Interpret if-conditions in "input_string" as "SCondition"s and store them   ######
######   in "Conditions".                                                            ######
######################################################################################## */
bool CPrompt::FindConditions(string &input_string, vector<SCondition> &Conditions) const
{
  if (Conditions.size() != 0)
  {
    cout << "Warning in bool CPrompt::FindConditions(...) const: Set of conditions is not empty. Now cleared." << endl;
    Conditions.clear();
  }
  
  const COrbifold      &Orbifold   = this->Orbifolds[this->OrbifoldIndex];
  const SSymmetryGroup &GaugeGroup = Orbifold.StandardConfig.SymmetryGroup;

  unsigned index = 0;
  bool condition_ok = true;
  string tmp_string = "";
  
  // begin: find conditions
  while (this->FindParameterType2(input_string, "if(", tmp_string))
  {
    condition_ok = true;
    SCondition NewCondition;
    NewCondition.ith_entry   = 0;
    NewCondition.value       = 0;
    NewCondition.field_label = "";

    // begin: left hand side of the condition
    // (p_sh)^2 of the left-moving shifted momentum p_sh of the field
    if (this->FindParameterType1(tmp_string, "length"))
      NewCondition.ConditionTypeId = 0;
    else
    // vev of the field
    if (this->FindParameterType1(tmp_string, "vev"))
      NewCondition.ConditionTypeId = 1;
    else
    // B-L charge of the field
    if (this->FindParameterType1(tmp_string, "B-L"))
      NewCondition.ConditionTypeId = 2;
    else
    // i = 0,1,2,.. i-th accidental U(1) charge of the field
    if (this->FindParameterType3(tmp_string, "acc. Q_", index))
    {
      NewCondition.ConditionTypeId = 4;
      NewCondition.ith_entry = index-1;
    }
    else
    // i = 1,2,... i-th U(1) charge of the field
    if (this->FindParameterType3(tmp_string, "Q_", index))
    {
      if ((index < 1) || (index > GaugeGroup.GaugeGroup.u1directions.size()))
      {
        condition_ok = false;
        (*this->Print.out) << "  " << this->Print.cbegin << "i-th U(1), i = " << index << " out of range." << this->Print.cend << endl;
      }
      
      NewCondition.ConditionTypeId = 3;
      NewCondition.ith_entry = index-1;
    }
    else
    // dimension of the irrep with respect to the i-th gauge group factor
    if (this->FindParameterType3(tmp_string, "rep_", index))
    {
      NewCondition.ConditionTypeId = 5;
      NewCondition.ith_entry = index-1;
    }
    else
    // i = 1, ..., 16 i-th entry of the left-moving shifted momentum p_sh of the field (first p_sh if non-Abelian representation)
    if (this->FindParameterType3(tmp_string, "p_sh_", index))
    {
      if ((index < 1) || (index > 16))
      {
        condition_ok = false;
        (*this->Print.out) << "  " << this->Print.cbegin << "p_sh_i, i = " << index << " out of range." << this->Print.cend << endl;
      }

      NewCondition.ConditionTypeId = 6;
      NewCondition.ith_entry = index-1;
    }
    else
    // i = 1,2,3,4 i-th entry of the fields q_sh-charge
    if (this->FindParameterType3(tmp_string, "q_sh_", index))
    {
      if ((index < 1) || (index > 4))
      {
        condition_ok = false;
        (*this->Print.out) << "  " << this->Print.cbegin << "q_sh_i, i = " << index << " out of range." << this->Print.cend << endl;
      }

      NewCondition.ConditionTypeId = 7;
      NewCondition.ith_entry = index-1;
    }
    else
    // number of string oscillators acing on the fields ground state
    if (this->FindParameterType1(tmp_string, "#osci."))
      NewCondition.ConditionTypeId = 8;
    else
    // field label
    if (this->FindParameterType1(tmp_string, "label"))
      NewCondition.ConditionTypeId = 9;
    else
    {
      condition_ok = false;
      (*this->Print.out) << "  " << this->Print.cbegin << "Condition \"" << tmp_string << "\": left hand side not known." << this->Print.cend << endl;
    }
    // end: left hand side of the condition

    // begin: equality sign of the condition
    if (this->FindParameterType1(tmp_string, " == "))
      NewCondition.ComparisonTypeId = 0;
    else
    if (this->FindParameterType1(tmp_string, " != "))
      NewCondition.ComparisonTypeId = 1;
    else
    if (this->FindParameterType1(tmp_string, " > "))
      NewCondition.ComparisonTypeId = 2;
    else
    if (this->FindParameterType1(tmp_string, " >= "))
      NewCondition.ComparisonTypeId = 3;
    else
    if (this->FindParameterType1(tmp_string, " < "))
      NewCondition.ComparisonTypeId = 4;
    else
    if (this->FindParameterType1(tmp_string, " <= "))
      NewCondition.ComparisonTypeId = 5;
    else
    if (this->FindParameterType1(tmp_string, " involves "))
      NewCondition.ComparisonTypeId = 6;
    else
    if (this->FindParameterType1(tmp_string, " !involves "))
      NewCondition.ComparisonTypeId = 7;
    else
    {
      condition_ok = false;
      (*this->Print.out) << "  " << this->Print.cbegin << "Condition \"" << tmp_string << "\": equality sign not known." << this->Print.cend << endl;
    }
    // end: equality sign of the condition

    // begin: right hand side of the condition
    if (this->FindParameterType1(tmp_string, "even/odd"))
    {
      if ((NewCondition.ComparisonTypeId != 0) && (NewCondition.ComparisonTypeId != 1))
      {
        (*this->Print.out) << "  " << this->Print.cbegin << "Condition \"" << tmp_string << "\": even/odd only works with == or != ." << this->Print.cend << endl;
        condition_ok = false;
      }
      NewCondition.ValueTypeId = 2;
    }
    else
    if (this->FindParameterType1(tmp_string, "even"))
    {
      if ((NewCondition.ComparisonTypeId != 0) && (NewCondition.ComparisonTypeId != 1))
      {
        (*this->Print.out) << "  " << this->Print.cbegin << "Condition \"" << tmp_string << "\": even only works with == or != ." << this->Print.cend << endl;
        condition_ok = false;
      }
      NewCondition.ValueTypeId = 0;
    }
    else
    if (this->FindParameterType1(tmp_string, "odd"))
    {
      if ((NewCondition.ComparisonTypeId != 0) && (NewCondition.ComparisonTypeId != 1))
      {
        (*this->Print.out) << "  " << this->Print.cbegin << "Condition \"" << tmp_string << "\": odd only works with == or != ." << this->Print.cend << endl;
        condition_ok = false;
      }
      NewCondition.ValueTypeId = 1;
    }
    else
    {
      if (tmp_string.find_first_not_of("+-0123456789/") != string::npos)
      {
        if (NewCondition.ConditionTypeId != 9)
        {
          (*this->Print.out) << "  " << this->Print.cbegin << "Condition \"" << tmp_string << "\": rational number expected." << this->Print.cend << endl;
          condition_ok = false;
        }
        else
        {
          NewCondition.field_label = tmp_string;
          NewCondition.ValueTypeId = 4;
        }
      }
      else
      {
        // if value is integer (i.e. not a rational number)
        if (tmp_string.find("/") == string::npos)
          NewCondition.value = rational<int>(atoi(tmp_string.c_str()), 1);
        // if value is a rational number
        else
        {
          istringstream myStream(tmp_string);

          if (!(myStream >> NewCondition.value))
          {
            (*this->Print.out) << "  " << this->Print.cbegin << "Condition \"" << tmp_string << "\": rational number expected." << this->Print.cend << endl;
            condition_ok = false;
          }
        }
        NewCondition.ValueTypeId = 3;
      }
    }
    // end: right hand side of the condition

    if ((NewCondition.ConditionTypeId == 9)
    && (((NewCondition.ComparisonTypeId != 0) && (NewCondition.ComparisonTypeId != 1) && (NewCondition.ComparisonTypeId != 6) && (NewCondition.ComparisonTypeId != 7)) || (NewCondition.ValueTypeId != 4)))
    {
      (*this->Print.out) << "  " << this->Print.cbegin << "Condition for field labels failed." << this->Print.cend << endl;
      condition_ok = false;
    }
    if ((NewCondition.ConditionTypeId != 9)
    && ((NewCondition.ComparisonTypeId == 6) || (NewCondition.ComparisonTypeId == 7) || (NewCondition.ValueTypeId == 4)))
    {
      (*this->Print.out) << "  " << this->Print.cbegin << "Condition only works for field labels." << this->Print.cend << endl;
      condition_ok = false;
    }

    if (condition_ok)
    {
      Conditions.push_back(NewCondition);
      /*cout << "new condition" << endl;
      cout << "ConditionTypeId:  " << NewCondition.ConditionTypeId << endl;
      cout << "ith_entry:        " << NewCondition.ith_entry << endl;
      cout << "ComparisonTypeId: " << NewCondition.ComparisonTypeId << endl;
      cout << "ValueTypeId:      " << NewCondition.ValueTypeId << endl;
      cout << "value:            " << NewCondition.value << endl;
      cout << "field_label:      " << NewCondition.field_label << endl;*/
    }
  }
  if (Conditions.size() != 0)
    return true;
  else
    return false;
}



/* ########################################################################################
######   ApplyConditions(vector<string> &Commands, unsigned &exec_command)           ######
######                                                                               ######
######   Version: 10.10.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Commands     : list of prompt commands                                   ######
######   2) exec_command : index of the command that shall be executed next          ######
######   output:                                                                     ######
######   return value    : did a problem occur?                                      ######
###########################################################################################
######   description:                                                                ######
######   Evaluate "@if(..)", "@else", "@endif".                                      ######
######################################################################################## */
bool CPrompt::ApplyConditions(vector<string> &Commands, unsigned &exec_command)
{
  if (exec_command >= Commands.size())
    return true;

  string current_command = Commands[exec_command];
  string condition = "";
  if (!this->FindCommandType1(current_command, "@if(", condition))
    return true;

  condition = "";
  vector<string> PartsOfCondition;

  const SConfig &VEVConfig = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]];

  bool condition_fulfilled = true;
  while (condition_fulfilled && this->FindParameterType2(current_command, "if(", condition))
  {
    condition_fulfilled = false;

    PartsOfCondition.clear();
    global_DecomposeString(condition, PartsOfCondition, " ");

    if (PartsOfCondition[0] == "set")
    {
      if (PartsOfCondition.size() == 4)
      {
        unsigned index = 0;
        if (this->MessageXNotKnown(VEVConfig.NamesOfSetsOfFields, "Set", PartsOfCondition[1], index))
          return false;

        if (PartsOfCondition[2] == "is")
        {
          if (PartsOfCondition[3] == "empty")
            condition_fulfilled = (VEVConfig.SetsOfFields[index].size() == 0);
          else
          if (PartsOfCondition[3] == "!empty")
            condition_fulfilled = (VEVConfig.SetsOfFields[index].size() != 0);
        }
      }
    }
    else
    if ((PartsOfCondition[0] == "hidden") && (PartsOfCondition[1] == "gauge") && (PartsOfCondition[2] == "group"))
    {
      string hidden_GG  = "";
      string comparison = "";
      unsigned index_from = 0;
      unsigned index_to   = VEVConfig.SymmetryGroup.GaugeGroup.factor.size();

      // for example: @if(hidden gauge group involves A1)
      if (PartsOfCondition.size() == 5)
      {
        comparison = PartsOfCondition[3];
        hidden_GG  = PartsOfCondition[4];
      }
      else
      // for example: @if(hidden gauge group from second E8 !involves E)
      if ((PartsOfCondition.size() == 8) && (PartsOfCondition[3] == "from") && (PartsOfCondition[5] == "E8"))
      {
        comparison = PartsOfCondition[6];
        hidden_GG  = PartsOfCondition[7];

        bool error = false;
        if ((PartsOfCondition[4] == "first"))
          index_to = VEVConfig.SymmetryGroup.Position_of_and_in_GaugeGroup;
        else
        if ((PartsOfCondition[4] == "second"))
          index_from = VEVConfig.SymmetryGroup.Position_of_and_in_GaugeGroup;
        else
          error = true;

        if (error || (this->Orbifolds[this->OrbifoldIndex].OrbifoldGroup.GetLattice() != E8xE8))
        {
          (*this->Print.out) << "  " << this->Print.cbegin << "Warning in @if condition: hidden gauge group cannot originate from E8." << this->Print.cend << endl;
          return false;
        }
      }
      string   algebra = "";
      unsigned rank    = 0;

      size_t pos = hidden_GG.find_first_of("0123456789", 0);

      const bool with_rank = (pos != string::npos);
      if (with_rank)
      {
        algebra = hidden_GG.substr(0,pos);
        rank = (unsigned)atoi((hidden_GG.substr(pos, string::npos)).c_str()) - 1;
        if (algebra == "E")
          ++rank;
      }
      else
        algebra = hidden_GG;

      if ((algebra != "SU") && (algebra != "SO") && (algebra != "E"))
      {
        (*this->Print.out) << "  " << this->Print.cbegin << "Warning in @if condition: Gauge group \"" << hidden_GG << "\" not of ADE type." << this->Print.cend << endl;
        return false;
      }

      if ((comparison != "involves") && (comparison != "!involves"))
      {
        (*this->Print.out) << "  " << this->Print.cbegin << "Warning in @if condition: Comparision must be \"involves\" or \"!involves\"." << this->Print.cend << endl;
        return false;
      }

      if (comparison == "!involves")
      {
        condition_fulfilled = true;
        for (unsigned i = index_from; condition_fulfilled && (i < index_to); ++i)
        {
          // analyse hidden gauge group factors only
          if (find(VEVConfig.SymmetryGroup.observable_sector_GGs.begin(), VEVConfig.SymmetryGroup.observable_sector_GGs.end(), i) == VEVConfig.SymmetryGroup.observable_sector_GGs.end())
          {
            const gaugeGroupFactor<double> &ggf = VEVConfig.SymmetryGroup.GaugeGroup.factor[i];

            if (((ggf.algebra[0] == 'A') && (algebra == "SU")) ||
                ((ggf.algebra[0] == 'D') && (algebra == "SO")) ||
                ((ggf.algebra[0] == 'E') && (algebra == "E")))
            {
              if (with_rank)
              {
                if (ggf.rank == rank)
                  condition_fulfilled = false;
              }
              else
                condition_fulfilled = false;
            }
          }
        }
      }
      else
      if (comparison == "involves")
      {
        condition_fulfilled = false;
        for (unsigned i = index_from; !condition_fulfilled && (i < index_to); ++i)
        {
          // analyse hidden gauge group factors only
          if (find(VEVConfig.SymmetryGroup.observable_sector_GGs.begin(), VEVConfig.SymmetryGroup.observable_sector_GGs.end(), i) == VEVConfig.SymmetryGroup.observable_sector_GGs.end())
          {
            const gaugeGroupFactor<double> &ggf = VEVConfig.SymmetryGroup.GaugeGroup.factor[i];

            if (((ggf.algebra[0] == 'A') && (algebra == "SU")) ||
                ((ggf.algebra[0] == 'D') && (algebra == "SO")) ||
                ((ggf.algebra[0] == 'E') && (algebra == "E")))
            {
              if (with_rank)
              {
                if (ggf.rank == rank)
                  condition_fulfilled = true;
              }
              else
                condition_fulfilled = true;
            }
          }
        }
      }
    }
    else
    if (PartsOfCondition[0] == "#fields")
    {
      if ((PartsOfCondition.size() == 6) && (PartsOfCondition[1] == "in") && (PartsOfCondition[2] == "set"))
      {
        unsigned index = 0;
        if (this->MessageXNotKnown(VEVConfig.NamesOfSetsOfFields, "Set", PartsOfCondition[3], index))
          return false;

        if (PartsOfCondition[5].find_first_not_of(" 0123456789") != string::npos)
        {
          if (this->print_output)
            (*this->Print.out) << "\n  " << this->Print.cbegin << PartsOfCondition[5] << " not a number." << this->Print.cend << "\n" << endl;

          return false;
        }

        const unsigned number_of_fields = (unsigned)atoi(PartsOfCondition[5].c_str());

        if (PartsOfCondition[4] == ">")
          condition_fulfilled = (VEVConfig.SetsOfFields[index].size() > number_of_fields);
        else
        if (PartsOfCondition[4] == "<")
          condition_fulfilled = (VEVConfig.SetsOfFields[index].size() < number_of_fields);
        else
        if (PartsOfCondition[4] == ">=")
          condition_fulfilled = (VEVConfig.SetsOfFields[index].size() >= number_of_fields);
        else
        if (PartsOfCondition[4] == "<=")
          condition_fulfilled = (VEVConfig.SetsOfFields[index].size() <= number_of_fields);
        else
        if (PartsOfCondition[4] == "==")
          condition_fulfilled = (VEVConfig.SetsOfFields[index].size() == number_of_fields);
        else
        if (PartsOfCondition[4] == "!=")
          condition_fulfilled = (VEVConfig.SetsOfFields[index].size() != number_of_fields);
      }
    }
  }
  ++exec_command;

  if (condition_fulfilled)
  {
    unsigned number_of_open_if = 1;

    for (unsigned i = exec_command; i < Commands.size(); ++i)
    {
      if (this->FindCommandType1(Commands[i], "@if(", condition))
        ++number_of_open_if;
      else
      if ((number_of_open_if == 1) && this->FindCommandType0(Commands[i], "@else"))
      {
        unsigned j = 0;
        for (j = i; (number_of_open_if != 0) && (j < Commands.size()); ++j)
        {
          if (this->FindCommandType1(Commands[j], "@if(", condition))
            ++number_of_open_if;

          if (this->FindCommandType0(Commands[j], "@endif"))
            --number_of_open_if;
        }
        Commands.erase(Commands.begin()+i, Commands.begin()+j);

        return true;
      }
      else
      if (this->FindCommandType0(Commands[i], "@endif"))
      {
        --number_of_open_if;

        if (number_of_open_if == 0)
        {
          Commands.erase(Commands.begin()+i);
          return true;
        }
      }
    }
  }
  else
  {
    unsigned number_of_open_if = 1;

    for (unsigned i = exec_command; i < Commands.size(); ++i)
    {
      if ((number_of_open_if == 1) && this->FindCommandType0(Commands[i], "@else"))
      {
        Commands.erase(Commands.begin()+exec_command, Commands.begin()+i+1);

        // search and delete corresponding "@endif"
        unsigned j = 0;
        for (j = exec_command; (number_of_open_if != 0) && (j < Commands.size()); ++j)
        {
          if (this->FindCommandType1(Commands[j], "@if(", condition))
            ++number_of_open_if;

          if (this->FindCommandType0(Commands[j], "@endif"))
            --number_of_open_if;
        }
        Commands.erase(Commands.begin()+j);

        return true;
      }
      else
      if (this->FindCommandType1(Commands[i], "@if(", condition))
        ++number_of_open_if;
      else
      if (this->FindCommandType0(Commands[i], "@endif"))
      {
        if (number_of_open_if == 1)
        {
          Commands.erase(Commands.begin()+exec_command, Commands.begin()+i+1);
          return true;
        }
        else
          --number_of_open_if;
      }
    }
  }

  return false;
}



/* ########################################################################################
######   ApplyConditions(const vector<SCondition> &Conditions, ...) const            ######
######                                                                               ######
######   Version: 19.08.2011                                                         ######
######   Check-Level: 1                                                              ######
######                                                                               ######
###########################################################################################
######   input:                                                                      ######
######   1) Conditions   : set of conditions to be imposed on the fields             ######
######   2) FieldIndices : vector of field indices; the ones that do not fulfill all ######
######                     conditions will be removed                                ######
######   output:                                                                     ######
######   return value    : did a problem occur?                                      ######
###########################################################################################
######   description:                                                                ######
######   Remove those field indices from "FieldIndices" that do not fulfill all      ######
######   conditions "Conditions".                                                    ######
######################################################################################## */
bool CPrompt::ApplyConditions(const vector<SCondition> &Conditions, vector<unsigned> &FieldIndices) const
{
  // Set the precision
  const double prec = 0.0001;

  const COrbifold       &Orbifold  = this->Orbifolds[this->OrbifoldIndex];
  const vector<CSector> &Sectors   = Orbifold.GetSectors();
  const SConfig         &VEVConfig = this->AllVEVConfigs[this->OrbifoldIndex][this->AllVEVConfigsIndex[this->OrbifoldIndex]];
  const vector<CField>  &Fields    = VEVConfig.Fields;

  unsigned j = 0;

  const size_t c1 = Conditions.size();
  if (c1 == 0)
    return true;

  bool equal = true;
  bool field_ok = true;
  double TestObject = 0.0;

  vector<unsigned> NewFieldIndices;

  const size_t s1 = FieldIndices.size();
  for (unsigned i = 0; i < s1; ++i)
  {
    const CField &Field = Fields[FieldIndices[i]];

    field_ok = true;
    for (j = 0; field_ok && (j < c1); ++j)
    {
      const SCondition &Condition = Conditions[j];

      // set "TestObject", specified by "ConditionTypeId"
      if (Condition.ConditionTypeId == 9)
      {
        const string &label = Field.Labels[VEVConfig.use_Labels];
        switch (Condition.ComparisonTypeId)
        {
          case 0: // equal
          {
            if (label != Condition.field_label)
              field_ok = false;
            break;
          }
          case 1: // not equal
          {
            if (label == Condition.field_label)
              field_ok = false;
            break;
          }
          case 6: // involves
          {
            if (label.find(Condition.field_label, 0) == string::npos)
              field_ok = false;
            break;
          }
          case 7: // not involves
          {
            if (label.find(Condition.field_label, 0) != string::npos)
              field_ok = false;
            break;
          }
        }
      }
      else
      {
        TestObject = 0.0;

        switch (Condition.ConditionTypeId)
        {
          case 0: // length
          {
            TestObject = Field.GetLMWeight(0, Sectors).GetSqrTo(16);
            break;
          }
          case 1: // vev
          {
            TestObject = Field.VEVs.GetLength();
            break;
          }
          case 2: // B-L
          {
            TestObject = Field.BmLCharge;
            break;
          }
          case 3: // Q_i
          {
            TestObject = Field.U1Charges[Condition.ith_entry];
            break;
          }
          case 4: // accidental Q_i
          {
            const rational<CHugeInt> &AccU1Charge = Field.AccU1Charges[Condition.ith_entry];
            TestObject = ((long double)AccU1Charge.numerator().ToLongLongInt())/((long double)AccU1Charge.denominator().ToLongLongInt());
            break;
          }
          case 5: // rep_i
          {
            TestObject = Field.Dimensions[Condition.ith_entry].Dimension;
            break;
          }
          case 6: // left-moving shifted momentum p_sh_i
          {
            TestObject = Field.GetLMWeight(0, Sectors)[Condition.ith_entry];
            break;
          }
          case 7: // R-charge R_i
          {

            TestObject = Field.q_sh[Condition.ith_entry];
            break;
          }
          case 8: // number of oscillators acting on the state
          {
            TestObject =  Field.GetNumberOfOscillators(Sectors);
            break;
          }
          default:
          {
            cout << "Warning in bool CPrompt::ApplyConditions(...) const: ConditionTypeId not known. Return false." << endl;
            return false;
          }
        }

        // == and !=
        if ((equal = (Condition.ComparisonTypeId == 0)) || (Condition.ComparisonTypeId == 1))
        {
          const rational<int> RationalTestObject = D2Rat(TestObject);

          switch (Condition.ValueTypeId)
          {
            case 0: // equal or not-equal to even
            {
              // field is not ok if "TestObject" is not an integer or not even
              if (equal && ((RationalTestObject.denominator() != 1) || (RationalTestObject.numerator() % 2 != 0)))
                field_ok = false;
              else
              // field is not ok if "TestObject" is even
              if (!equal && (RationalTestObject.denominator() == 1) && (RationalTestObject.numerator() % 2 == 0))
                field_ok = false;

              break;
            }
            case 1: // equal or not-equal to odd
            {
              // field is not ok if "TestObject" is not an integer or even
              if (equal && ((RationalTestObject.denominator() != 1) || (RationalTestObject.numerator() % 2 == 0)))
                field_ok = false;
              else
              // field is not ok if "TestObject" is odd
              if (!equal && (RationalTestObject.denominator() == 1) && (RationalTestObject.numerator() % 2 != 0))
                field_ok = false;

              break;
            }
            case 2: // equal or not-equal to (even/odd)
            {
              // field is not ok if "TestObject" is the numerator is odd or the denominator is even
              if (equal && ((RationalTestObject.numerator() % 2 != 0) || (RationalTestObject.denominator() % 2 == 0)))
                field_ok = false;
              else
              // field is not ok if "TestObject" is the numerator is even and the denominator is odd
              if (!equal && (RationalTestObject.numerator() % 2 == 0) && (RationalTestObject.denominator() % 2 != 0))
                field_ok = false;

              break;
            }
            case 3: // equal or not-equal to rational number
            {
              rational<int> diff = RationalTestObject - Condition.value;
              long double tmp_diff = fabs(((long double)diff.numerator())/((long double)diff.denominator()));

              // field is not ok if "TestObject" is not equal to "value"
              if (equal && (tmp_diff > prec))
                field_ok = false;
              else
              // field is not ok if "TestObject" is equal to "value"
              if (!equal && (tmp_diff < prec))
                field_ok = false;

              break;
            }
            default:
            {
              cout << "Warning in bool CPrompt::ApplyConditions(...) const: ValueTypeId not known. Return false." << endl;
              return false;
            }
          }
        }
        // the inequalities only work with a rational number to compare
        else
        {
          if (Condition.ValueTypeId != 3)
          {
            cout << "Warning in bool CPrompt::ApplyConditions(...) const: Inequalities only work with rational numbers. Return false." << endl;
            return false;
          }
          long double value = ((long double)Condition.value.numerator())/((long double)Condition.value.denominator());

          // >
          if (Condition.ComparisonTypeId == 2)
          {
            if ((fabs(TestObject - value) < prec) || (TestObject + prec < value))
              field_ok = false;
          }
          else
          // >=
          if (Condition.ComparisonTypeId == 3)
          {
            if (TestObject + prec < value)
              field_ok = false;
          }
          else
          // <
          if (Condition.ComparisonTypeId == 4)
          {
            if ((fabs(TestObject - value) < prec) || (TestObject > value + prec))
              field_ok = false;
          }
          else
          // <=
          if (Condition.ComparisonTypeId == 5)
          {
            if (TestObject > value + prec)
              field_ok = false;
          }
        }
      }
    }
    if (field_ok)
      NewFieldIndices.push_back(FieldIndices[i]);
  }
  FieldIndices = NewFieldIndices;
  return true;
}
