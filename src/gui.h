#include <proto/dos.h>

/**
 * Create the GUI
 * @return RETURN_OK on success, RETURN_ERROR on failure
**/
LONG initVideo();

/**
 * The main run loop of the GUI
 * @return The return code of the application
 * @see RETURN_OK
 * @see RETURN_ERROR
**/ 
LONG startGUIRunLoop();

/**
 * Shutdown the GUI
**/
void shutdownGUI();

/**
 * Display an error message
 * @param message the message to display
**/ 
void displayError(STRPTR message);

/**
 * Update the status bar
 * @param message the message to display
 * @param pen the pen to use for the text
 * 
**/ 
void updateStatusBar(CONST_STRPTR message, const ULONG pen);

/**
 * Display an error message about a disk error
 * @param message the message to display
 * @param error the error code returned by IOErr()
**/ 
void displayDiskError(STRPTR message, LONG error);