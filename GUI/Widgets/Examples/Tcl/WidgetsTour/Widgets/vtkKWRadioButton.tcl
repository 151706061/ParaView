proc vtkKWRadioButtonEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create two radiobuttons. 
  # They share the same variable name and each one has a different internal
  # value.

  vtkKWRadioButton radiob1
  radiob1 SetParent $parent
  radiob1 Create
  radiob1 SetText "A radiobutton"
  radiob1 SetValueAsInt 123

  vtkKWRadioButton radiob1b
  radiob1b SetParent $parent
  radiob1b Create
  radiob1b SetText "Another radiobutton"
  radiob1b SetValueAsInt 456

  radiob1 SetSelectedState 1
  radiob1b SetVariableName [radiob1 GetVariableName] 

  pack [radiob1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2
  pack [radiob1b GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create two radiobuttons. Use icons
  # They share the same variable name and each one has a different internal
  # value.

  vtkKWRadioButton radiob2
  radiob2 SetParent $parent
  radiob2 Create
  radiob2 SetImageToPredefinedIcon 100
  radiob2 IndicatorVisibilityOff
  radiob2 SetValue "foo"

  vtkKWRadioButton radiob2b
  radiob2b SetParent $parent
  radiob2b Create
  radiob2b SetImageToPredefinedIcon 64
  radiob2b IndicatorVisibilityOff
  radiob2b SetValue "bar"

  radiob2 SetSelectedState 1
  radiob2b SetVariableName [radiob2 GetVariableName] 

  pack [radiob2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2
  pack [radiob2b GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create a set of radiobutton
  # An easy way to create a bunch of related widgets without allocating
  # them one by one

  vtkKWRadioButtonSet radiob_set
  radiob_set SetParent $parent
  radiob_set Create
  radiob_set SetBorderWidth 2
  radiob_set SetReliefToGroove

  for {set id 0} {$id < 4} {incr id} {

    set radiob [radiob_set AddWidget $id] 
    $radiob SetText "Radiobutton $id"
    $radiob SetBalloonHelpString \
      "This radiobutton is part of a unique set a vtkKWRadioButtonSet,\
      which provides an easy way to create a bunch of related widgets\
      without allocating them one by one. The widgets can be layout as a\
      NxM grid. This classes automatically set the same variable name\
      among all radiobuttons as well as a unique value."
    }
  
  [radiob_set GetWidget 0] SetSelectedState 1

  pack [radiob_set GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # TODO: use vtkKWRadioButtonSetWithLabel and callbacks

  return "TypeCore"
}

proc vtkKWRadioButtonFinalizePoint {} {
  radiob1 Delete
  radiob1b Delete
  radiob2 Delete
  radiob2b Delete
  radiob_set Delete
}

