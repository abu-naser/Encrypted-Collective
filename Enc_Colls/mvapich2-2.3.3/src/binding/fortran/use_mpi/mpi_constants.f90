!     -*- Mode: Fortran; -*-
!  (C) 2008 by Argonne National Laboratory.
!       See COPYRIGHT in top-level directory.
        MODULE MPI_CONSTANTS
        IMPLICIT NONE
        INCLUDE 'mpifnoext.h'
        TYPE :: MPI_Status
           SEQUENCE
           INTEGER :: MPI_SOURCE, MPI_TAG, MPI_ERROR
           INTEGER :: count_lo
           INTEGER :: count_hi_and_cancelled
        END TYPE MPI_Status
       TYPE :: MPI_Info
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Info
       TYPE :: MPI_Message
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Message
       TYPE :: MPI_Op
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Op
       TYPE :: MPI_File
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_File
       TYPE :: MPI_Group
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Group
       TYPE :: MPI_Datatype
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Datatype
       TYPE :: MPI_Win
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Win
       TYPE :: MPI_Comm
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Comm
       TYPE :: MPI_Request
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Request
       TYPE :: MPI_Errhandler
          SEQUENCE
          INTEGER :: MPI_VAL
       END TYPE MPI_Errhandler
        INTERFACE OPERATOR(.EQ.)
            MODULE PROCEDURE infoeq
            MODULE PROCEDURE messageeq
            MODULE PROCEDURE opeq
            MODULE PROCEDURE fileeq
            MODULE PROCEDURE groupeq
            MODULE PROCEDURE datatypeeq
            MODULE PROCEDURE wineq
            MODULE PROCEDURE commeq
            MODULE PROCEDURE requesteq
            MODULE PROCEDURE errhandlereq
        END INTERFACE
        INTERFACE OPERATOR(.NE.)
            MODULE PROCEDURE infoneq
            MODULE PROCEDURE messageneq
            MODULE PROCEDURE opneq
            MODULE PROCEDURE fileneq
            MODULE PROCEDURE groupneq
            MODULE PROCEDURE datatypeneq
            MODULE PROCEDURE winneq
            MODULE PROCEDURE commneq
            MODULE PROCEDURE requestneq
            MODULE PROCEDURE errhandlerneq
        END INTERFACE
        CONTAINS
            LOGICAL FUNCTION infoeq(lhs,rhs)
            TYPE(MPI_Info), INTENT(IN) :: lhs, rhs
            infoeq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION infoeq
            LOGICAL FUNCTION infoneq(lhs,rhs)
            TYPE(MPI_Info), INTENT(IN) :: lhs, rhs
            infoneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION infoneq
            LOGICAL FUNCTION messageeq(lhs,rhs)
            TYPE(MPI_Message), INTENT(IN) :: lhs, rhs
            messageeq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION messageeq
            LOGICAL FUNCTION messageneq(lhs,rhs)
            TYPE(MPI_Message), INTENT(IN) :: lhs, rhs
            messageneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION messageneq
            LOGICAL FUNCTION opeq(lhs,rhs)
            TYPE(MPI_Op), INTENT(IN) :: lhs, rhs
            opeq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION opeq
            LOGICAL FUNCTION opneq(lhs,rhs)
            TYPE(MPI_Op), INTENT(IN) :: lhs, rhs
            opneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION opneq
            LOGICAL FUNCTION fileeq(lhs,rhs)
            TYPE(MPI_File), INTENT(IN) :: lhs, rhs
            fileeq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION fileeq
            LOGICAL FUNCTION fileneq(lhs,rhs)
            TYPE(MPI_File), INTENT(IN) :: lhs, rhs
            fileneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION fileneq
            LOGICAL FUNCTION groupeq(lhs,rhs)
            TYPE(MPI_Group), INTENT(IN) :: lhs, rhs
            groupeq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION groupeq
            LOGICAL FUNCTION groupneq(lhs,rhs)
            TYPE(MPI_Group), INTENT(IN) :: lhs, rhs
            groupneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION groupneq
            LOGICAL FUNCTION datatypeeq(lhs,rhs)
            TYPE(MPI_Datatype), INTENT(IN) :: lhs, rhs
            datatypeeq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION datatypeeq
            LOGICAL FUNCTION datatypeneq(lhs,rhs)
            TYPE(MPI_Datatype), INTENT(IN) :: lhs, rhs
            datatypeneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION datatypeneq
            LOGICAL FUNCTION wineq(lhs,rhs)
            TYPE(MPI_Win), INTENT(IN) :: lhs, rhs
            wineq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION wineq
            LOGICAL FUNCTION winneq(lhs,rhs)
            TYPE(MPI_Win), INTENT(IN) :: lhs, rhs
            winneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION winneq
            LOGICAL FUNCTION commeq(lhs,rhs)
            TYPE(MPI_Comm), INTENT(IN) :: lhs, rhs
            commeq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION commeq
            LOGICAL FUNCTION commneq(lhs,rhs)
            TYPE(MPI_Comm), INTENT(IN) :: lhs, rhs
            commneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION commneq
            LOGICAL FUNCTION requesteq(lhs,rhs)
            TYPE(MPI_Request), INTENT(IN) :: lhs, rhs
            requesteq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION requesteq
            LOGICAL FUNCTION requestneq(lhs,rhs)
            TYPE(MPI_Request), INTENT(IN) :: lhs, rhs
            requestneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION requestneq
            LOGICAL FUNCTION errhandlereq(lhs,rhs)
            TYPE(MPI_Errhandler), INTENT(IN) :: lhs, rhs
            errhandlereq = lhs%MPI_VAL .EQ. rhs%MPI_VAL
            END FUNCTION errhandlereq
            LOGICAL FUNCTION errhandlerneq(lhs,rhs)
            TYPE(MPI_Errhandler), INTENT(IN) :: lhs, rhs
            errhandlerneq = lhs%MPI_VAL .NE. rhs%MPI_VAL
            END FUNCTION errhandlerneq
        END MODULE MPI_CONSTANTS
