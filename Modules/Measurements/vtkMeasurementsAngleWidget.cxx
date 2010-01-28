#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"

#include "vtkMeasurementsAngleWidget.h"

#include "vtkKWFrameWithLabel.h"
#include "vtkKWFrame.h"
#include "vtkKWCheckButton.h"
#include "vtkSlicerNodeSelectorWidget.h"
#include "vtkKWCheckButtonWithLabel.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"



#include "vtkAngleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkAngleRepresentation3D.h"
#include "vtkPolygonalSurfacePointPlacer.h"

#include "vtkMeasurementsAngleWidgetClass.h"

#include "vtkMRMLMeasurementsAngleNode.h"

#include "vtkSlicerViewerWidget.h"
#include "vtkSlicerApplication.h"
#include "vtkSlicerApplicationGUI.h"


#include "vtkMRMLTransformNode.h"
#include "vtkMRMLLinearTransformNode.h"

class vtkMeasurementsAngleWidgetCallback : public vtkCommand
{
public:
  static vtkMeasurementsAngleWidgetCallback *New()
  { return new vtkMeasurementsAngleWidgetCallback; }
  virtual void Execute (vtkObject *caller, unsigned long event, void*)
  {
    // save node for undo if it's the start of an interaction event
    if (event == vtkCommand::StartInteractionEvent)
      {
      if (this->AngleNode && this->AngleNode->GetScene())
        {
        this->AngleNode->GetScene()->SaveStateForUndo(this->AngleNode);
        }
      }
    else if (event == vtkCommand::InteractionEvent)
      {
      vtkAngleWidget *angleWidget = reinterpret_cast<vtkAngleWidget*>(caller);
      if (angleWidget)
        {
        if (angleWidget->GetRepresentation())
          {
          vtkAngleRepresentation3D *rep = vtkAngleRepresentation3D::SafeDownCast(angleWidget->GetRepresentation());
          if (rep)
            {
            double p1[3], p2[3], p3[3];
            rep->GetPoint1WorldPosition(p1);
            rep->GetPoint2WorldPosition(p2);
            rep->GetCenterWorldPosition(p3);
            if (this->AngleNode)
              {
              // does the angle node have a transform?
              vtkMRMLTransformNode* tnode = this->AngleNode->GetParentTransformNode();
              vtkMatrix4x4* transformToWorld = vtkMatrix4x4::New();
              transformToWorld->Identity();
              if (tnode != NULL && tnode->IsLinear())
                {
                vtkMRMLLinearTransformNode *lnode = vtkMRMLLinearTransformNode::SafeDownCast(tnode);
                lnode->GetMatrixTransformToWorld(transformToWorld);
                }
              // convert by the inverted parent transform
              double  xyzw[4];
              xyzw[0] = p1[0];
              xyzw[1] = p1[1];
              xyzw[2] = p1[2];
              xyzw[3] = 1.0;
              double worldxyz[4], *worldp = &worldxyz[0];
              transformToWorld->Invert();
              transformToWorld->MultiplyPoint(xyzw, worldp);
              this->AngleNode->SetPosition1(worldxyz[0], worldxyz[1], worldxyz[2]);
              // second point
              xyzw[0] = p2[0];
              xyzw[1] = p2[1];
              xyzw[2] = p2[2];
              xyzw[3] = 1.0;
              transformToWorld->MultiplyPoint(xyzw, worldp);
              this->AngleNode->SetPosition2(worldxyz[0], worldxyz[1], worldxyz[2]);
              // center point
              xyzw[0] = p3[0];
              xyzw[1] = p3[1];
              xyzw[2] = p3[2];
              xyzw[3] = 1.0;
              transformToWorld->MultiplyPoint(xyzw, worldp);
              this->AngleNode->SetPositionCenter(worldxyz[0], worldxyz[1], worldxyz[2]);
              
              transformToWorld->Delete();
              transformToWorld = NULL;
              tnode = NULL;
              }
            }
          }
        }
      }
  }
  //,AngleRepresentation(0)
  vtkMeasurementsAngleWidgetCallback():AngleNode(0) {}
  vtkMRMLMeasurementsAngleNode *AngleNode;
//  std::string AngleID;
//  vtkAngleRepresentation3D *AngleRepresentation;
};

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMeasurementsAngleWidget );
vtkCxxRevisionMacro ( vtkMeasurementsAngleWidget, "$Revision: 1.0 $");


//---------------------------------------------------------------------------
vtkMeasurementsAngleWidget::vtkMeasurementsAngleWidget ( )
{

  // gui elements
  this->AngleNodeID = NULL;
  this->AngleSelectorWidget = NULL;
  this->VisibilityButton = NULL;
  this->Ray1VisibilityButton = NULL;
  this->Ray2VisibilityButton = NULL;
  this->ArcVisibilityButton = NULL;
  this->AngleModel1SelectorWidget = NULL;
  this->AngleModel2SelectorWidget = NULL;
  this->AngleModelCenterSelectorWidget = NULL;
  this->PointColourButton = NULL;
  this->LineColourButton = NULL;
  this->TextColourButton = NULL;
 
  this->Position1Label = NULL;
  this->Position1XEntry = NULL;
  this->Position1YEntry = NULL;
  this->Position1ZEntry = NULL;

  this->Position2Label = NULL;
  this->Position2XEntry = NULL;
  this->Position2YEntry = NULL;
  this->Position2ZEntry = NULL;

  this->PositionCenterLabel = NULL;
  this->PositionCenterXEntry = NULL;
  this->PositionCenterYEntry = NULL;
  this->PositionCenterZEntry = NULL;

  
  this->LabelFormatEntry = NULL;
  this->LabelScaleEntry = NULL;
  this->LabelVisibilityButton = NULL;

  this->Ray1VisibilityButton = NULL;
  this->Ray2VisibilityButton = NULL;
  this->ArcVisibilityButton = NULL;

  this->ResolutionEntry = NULL;

  this->AllVisibilityMenuButton = NULL;

  // 3d elements
  this->ViewerWidget = NULL;

//  this->AngleWidget = vtkMeasurementsAngleWidgetClass::New();
  
  
 
  
  this->SetAngleNodeID(NULL);
  
  this->Updating3DWidget = 0;

//  this->DebugOn();
}


//---------------------------------------------------------------------------
vtkMeasurementsAngleWidget::~vtkMeasurementsAngleWidget ( )
{
  this->RemoveMRMLObservers();
  this->RemoveWidgetObservers();

  // gui elements
  if ( this->AllVisibilityMenuButton )
    {
    this->AllVisibilityMenuButton->SetParent ( NULL );
    this->AllVisibilityMenuButton->Delete();
    this->AllVisibilityMenuButton = NULL;
    }
  if (this->AngleSelectorWidget)
    {
    this->AngleSelectorWidget->SetParent(NULL);
    this->AngleSelectorWidget->SetMRMLScene(NULL);
    this->AngleSelectorWidget->Delete();
    this->AngleSelectorWidget = NULL;
    }

  if (this->VisibilityButton)
    {
    this->VisibilityButton->SetParent(NULL);
    this->VisibilityButton->Delete();
    this->VisibilityButton = NULL;
    }

  if (this->Ray1VisibilityButton)
    {
    this->Ray1VisibilityButton->SetParent(NULL);
    this->Ray1VisibilityButton->Delete();
    this->Ray1VisibilityButton = NULL;
    }
  if (this->Ray2VisibilityButton)
    {
    this->Ray2VisibilityButton->SetParent(NULL);
    this->Ray2VisibilityButton->Delete();
    this->Ray2VisibilityButton = NULL;
    }
  if (this->ArcVisibilityButton)
    {
    this->ArcVisibilityButton->SetParent(NULL);
    this->ArcVisibilityButton->Delete();
    this->ArcVisibilityButton = NULL;
    }
  
  if (this->AngleModel1SelectorWidget)
    {
    this->AngleModel1SelectorWidget->SetParent(NULL);
    this->AngleModel1SelectorWidget->SetMRMLScene(NULL);
    this->AngleModel1SelectorWidget->Delete();
    this->AngleModel1SelectorWidget = NULL;
    }
  if (this->AngleModel2SelectorWidget)
    {
    this->AngleModel2SelectorWidget->SetParent(NULL);
    this->AngleModel2SelectorWidget->SetMRMLScene(NULL);
    this->AngleModel2SelectorWidget->Delete();
    this->AngleModel2SelectorWidget = NULL;
    }
  if (this->AngleModelCenterSelectorWidget)
    {
    this->AngleModelCenterSelectorWidget->SetParent(NULL);
    this->AngleModelCenterSelectorWidget->SetMRMLScene(NULL);
    this->AngleModelCenterSelectorWidget->Delete();
    this->AngleModelCenterSelectorWidget = NULL;
    }
  if (this->PointColourButton)
    {
    this->PointColourButton->SetParent(NULL);
    this->PointColourButton->Delete();
    this->PointColourButton= NULL;
    }
  if (this->LineColourButton)
    {
    this->LineColourButton->SetParent(NULL);
    this->LineColourButton->Delete();
    this->LineColourButton= NULL;
    }
  if (this->TextColourButton)
    {
    this->TextColourButton->SetParent(NULL);
    this->TextColourButton->Delete();
    this->TextColourButton= NULL;
    }
  if (this->Position1Label)
    {
    this->Position1Label->SetParent(NULL);
    this->Position1Label->Delete();
    this->Position1Label = NULL;
    }
  if (this->Position1XEntry)
    {
    this->Position1XEntry->SetParent(NULL);
    this->Position1XEntry->Delete();
    this->Position1XEntry = NULL;
    }
  if (this->Position1YEntry)
    {
    this->Position1YEntry->SetParent(NULL);
    this->Position1YEntry->Delete();
    this->Position1YEntry = NULL;
    }
  if (this->Position1ZEntry)
    {
    this->Position1ZEntry->SetParent(NULL);
    this->Position1ZEntry->Delete();
    this->Position1ZEntry = NULL;
    }

  if (this->Position2Label)
    {
    this->Position2Label->SetParent(NULL);
    this->Position2Label->Delete();
    this->Position2Label = NULL;
    }

  if (this->Position2XEntry)
    {
    this->Position2XEntry->SetParent(NULL);
    this->Position2XEntry->Delete();
    this->Position2XEntry = NULL;
    }
  if (this->Position2YEntry)
    {
    this->Position2YEntry->SetParent(NULL);
    this->Position2YEntry->Delete();
    this->Position2YEntry = NULL;
    }
  if (this->Position2ZEntry)
    {
    this->Position2ZEntry->SetParent(NULL);
    this->Position2ZEntry->Delete();
    this->Position2ZEntry = NULL;
    }

  if (this->PositionCenterLabel)
    {
    this->PositionCenterLabel->SetParent(NULL);
    this->PositionCenterLabel->Delete();
    this->PositionCenterLabel = NULL;
    }

  if (this->PositionCenterXEntry)
    {
    this->PositionCenterXEntry->SetParent(NULL);
    this->PositionCenterXEntry->Delete();
    this->PositionCenterXEntry = NULL;
    }
  if (this->PositionCenterYEntry)
    {
    this->PositionCenterYEntry->SetParent(NULL);
    this->PositionCenterYEntry->Delete();
    this->PositionCenterYEntry = NULL;
    }
  if (this->PositionCenterZEntry)
    {
    this->PositionCenterZEntry->SetParent(NULL);
    this->PositionCenterZEntry->Delete();
    this->PositionCenterZEntry = NULL;
    }

  if (this->LabelFormatEntry)
    {
    this->LabelFormatEntry->SetParent(NULL);
    this->LabelFormatEntry->Delete();
    this->LabelFormatEntry = NULL;
    }
  if (this->LabelScaleEntry)
    {
    this->LabelScaleEntry->SetParent(NULL);
    this->LabelScaleEntry->Delete();
    this->LabelScaleEntry = NULL;
    }
  if (this->LabelVisibilityButton)
    {
    this->LabelVisibilityButton->SetParent(NULL);
    this->LabelVisibilityButton->Delete();
    this->LabelVisibilityButton = NULL;
    }

  if (this->ResolutionEntry)
    {
    this->ResolutionEntry->SetParent(NULL);
    this->ResolutionEntry->Delete();
    this->ResolutionEntry = NULL;
    }

  // 3d widgets
  std::map<std::string, vtkMeasurementsAngleWidgetClass *>::iterator iter;
  for (iter = this->AngleWidgets.begin();
       iter != this->AngleWidgets.end();
       iter++)
    {
    iter->second->Delete();
    }
  this->AngleWidgets.clear(); 

  this->SetAngleNodeID(NULL);
  this->SetViewerWidget(NULL);
  this->SetMRMLScene ( NULL );
  
}


//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::PrintSelf ( ostream& os, vtkIndent indent )
{
    this->vtkObject::PrintSelf ( os, indent );

    os << indent << "vtkMeasurementsAngleWidget: " << this->GetClassName ( ) << "\n";
    // print widgets?
    std::map<std::string, vtkMeasurementsAngleWidgetClass *>::iterator iter;
    for (iter = this->AngleWidgets.begin(); iter !=  this->AngleWidgets.end(); iter++)
      {
      os << indent << "Angle Widget: " << iter->first.c_str() << "\n";
      iter->second->GetWidget()->PrintSelf(os, indent);
      }
}


//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::SetAngleNodeID ( char *id )
{
  if (this->GetAngleNodeID() != NULL &&
      id != NULL &&
      strcmp(id,this->GetAngleNodeID()) == 0)
    {
    vtkDebugMacro("SetAngleNodeID: no change in id, not doing anything for now: " << id << endl);
    return;
    }
  
  // get the old node - needed to remove events from it
//  vtkMRMLMeasurementsAngleNode *oldAngle = vtkMRMLMeasurementsAngleNode::SafeDownCast(this->MRMLScene->GetNodeByID(this->GetAngleNodeID()));
 
  // set the id properly - see the vtkSetStringMacro
  this->AngleNodeID = id;
  
  if (id == NULL)
    {
    vtkDebugMacro("SetAngleNodeID: NULL input id, clearing GUI and returning.\n");
    this->UpdateWidget(NULL);
    return;
    }
  
  // get the new node
  vtkMRMLMeasurementsAngleNode *newAngle = vtkMRMLMeasurementsAngleNode::SafeDownCast(this->MRMLScene->GetNodeByID(this->GetAngleNodeID()));
  // set up observers on the new node - now done in addanglewidget
  if (newAngle != NULL)
    {
//    vtkIntArray *events = vtkIntArray::New();
//    events->InsertNextValue(vtkCommand::ModifiedEvent);
//    events->InsertNextValue(vtkMRMLMeasurementsAngleNode::DisplayModifiedEvent);
//    events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
//    events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
//    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
//    vtkSetAndObserveMRMLNodeEventsMacro(oldAngle, newAngle, events);
//    events->Delete();

    // set up the GUI
    this->UpdateWidget(newAngle);
    }
  else
    {
    vtkDebugMacro ("ERROR: unable to get the mrml angle node to observe!\n");
    }
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::ProcessWidgetEvents(vtkObject *caller,
                                                     unsigned long event,
                                                     void *vtkNotUsed(callData) )
{
  vtkSlicerApplication *app = vtkSlicerApplication::SafeDownCast (this->GetApplication() );
  if ( !app )
    {
    vtkErrorMacro ( "ProcessWidgetEvents: got Null SlicerApplication" );
    return;
    }
  vtkSlicerApplicationGUI *appGUI = app->GetApplicationGUI();
  if ( !appGUI )
    {
    vtkErrorMacro ( "ProcessWidgetEvents: got Null SlicerApplicationGUI" );
    return;
    }

  // process events that apply to all lists
  vtkKWMenu *menu = vtkKWMenu::SafeDownCast ( caller );
  if (menu != NULL)
    {
    if ( menu == this->AllVisibilityMenuButton->GetMenu() )
      {
      // set visibility on all angles
      if ( menu->GetItemSelectedState ( "All Angles Visible" ) == 1 )
        {
        this->ModifyAllAngleVisibility (1 );
        }
      else if ( menu->GetItemSelectedState ( "All Angles Invisible" ) == 1 )
        {
        this->ModifyAllAngleVisibility (0 );
        }
      // call the update here as modifying the mrml nodes will bounce on the
      // check in process mrml events for process this widget event
      this->Update3DWidgetsFromMRML();
      }
    }
  // process angle node selector events
  if (this->AngleSelectorWidget ==  vtkSlicerNodeSelectorWidget::SafeDownCast(caller) &&
      event == vtkSlicerNodeSelectorWidget::NodeSelectedEvent )
      {
      vtkDebugMacro("ProcessWidgetEvents Node Selector Event: " << event << ".\n");
      vtkMRMLMeasurementsAngleNode *angleNode =
        vtkMRMLMeasurementsAngleNode::SafeDownCast(this->AngleSelectorWidget->GetSelected());
      if (angleNode != NULL)
        {
        this->SetAngleNodeID(angleNode->GetID());
        }
      else
        {
        vtkDebugMacro("ProcessWidgetEvent: the selected node is null!");
        }
      return;
    }

  // get the currently displayed list
  
  // is there one list?
  vtkMRMLMeasurementsAngleNode *activeAngleNode = (vtkMRMLMeasurementsAngleNode *)this->MRMLScene->GetNodeByID(this->GetAngleNodeID());
  if (activeAngleNode == NULL)
    {
    vtkDebugMacro("No selected angle");
    return;
    /*
      not allowing spontaneous creation of a angle
    vtkDebugMacro ("ERROR: No angle node, adding one first!\n");
      vtkMRMLMeasurementsAngleNode *newList = this->GetLogic()->AddAngle();      
      if (newList != NULL)
        {
        this->SetAngleNodeID(newList->GetID());
        newList->Delete();
        }
      else
        {
        vtkErrorMacro("Unable to add a new angle via the logic\n");
        }
      // now get the newly active node 
      activeAngleNode = (vtkMRMLMeasurementsAngleNode *)this->MRMLScene->GetNodeByID(this->GetAngleNodeID());
      if (activeAngleNode == NULL)
        {
        vtkErrorMacro ("ERROR adding a new angle list for the point...\n");
        return;
        }
    */
    }

  // GUI elements
  vtkKWCheckButton *b = vtkKWCheckButton::SafeDownCast ( caller );
  vtkKWChangeColorButton *ccbutton = vtkKWChangeColorButton::SafeDownCast(caller);
  vtkKWEntry *entry = vtkKWEntry::SafeDownCast(caller);
 
  if (b && event == vtkKWCheckButton::SelectedStateChangedEvent)
    {
    if (b == this->VisibilityButton->GetWidget()) 
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetVisibility(this->VisibilityButton->GetWidget()->GetSelectedState());
      this->Update3DWidget(activeAngleNode);
      }
    if (b == this->Ray1VisibilityButton->GetWidget()) 
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetRay1Visibility(this->Ray1VisibilityButton->GetWidget()->GetSelectedState());
      this->Update3DWidget(activeAngleNode);
      }
    if (b == this->Ray2VisibilityButton->GetWidget()) 
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetRay2Visibility(this->Ray2VisibilityButton->GetWidget()->GetSelectedState());
      this->Update3DWidget(activeAngleNode);
      }
    if (b == this->ArcVisibilityButton->GetWidget()) 
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetArcVisibility(this->ArcVisibilityButton->GetWidget()->GetSelectedState());
      this->Update3DWidget(activeAngleNode);
      }
    else if (b == this->LabelVisibilityButton->GetWidget() )
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetLabelVisibility(this->LabelVisibilityButton->GetWidget()->GetSelectedState());
      this->Update3DWidget(activeAngleNode);
      }
    }
  else if (vtkSlicerNodeSelectorWidget::SafeDownCast(caller) == this->AngleModel1SelectorWidget &&
           event == vtkSlicerNodeSelectorWidget::NodeSelectedEvent
           && this->GetViewerWidget())
    {
    vtkMRMLModelNode *model = 
      vtkMRMLModelNode::SafeDownCast(this->AngleModel1SelectorWidget->GetSelected());
    if (model != NULL  && model->GetDisplayNode() != NULL)
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetModelID1(model->GetID());
      this->Update3DWidget(activeAngleNode);
      }
    else
      {
      // is it a slice node?
      vtkMRMLSliceNode *slice = vtkMRMLSliceNode::SafeDownCast(this->AngleModel1SelectorWidget->GetSelected());
      if (slice != NULL && slice->GetID())
        {
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetModelID1(slice->GetID());
        this->Update3DWidget(activeAngleNode);
        }
      else
        {
        // remove the constraint by setting it to null
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetModelID1(NULL);
        this->Update3DWidget(activeAngleNode);
        }
      }
    }
  else if (vtkSlicerNodeSelectorWidget::SafeDownCast(caller) == this->AngleModel2SelectorWidget &&
           event == vtkSlicerNodeSelectorWidget::NodeSelectedEvent
           && this->GetViewerWidget())
    {
    vtkMRMLModelNode *model = 
      vtkMRMLModelNode::SafeDownCast(this->AngleModel2SelectorWidget->GetSelected());
    if (model != NULL  && model->GetDisplayNode() != NULL)
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetModelID2(model->GetID());
      this->Update3DWidget(activeAngleNode);
      }
    else
      {
      // is it a slice node?
      vtkMRMLSliceNode *slice = vtkMRMLSliceNode::SafeDownCast(this->AngleModel2SelectorWidget->GetSelected());
      if (slice != NULL && slice->GetID())
        {
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetModelID2(slice->GetID());
        this->Update3DWidget(activeAngleNode);
        }
      else
        {
        // remove the constraint by setting it to null
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetModelID2(NULL);
        this->Update3DWidget(activeAngleNode);
        }
      }
    }
  else if (vtkSlicerNodeSelectorWidget::SafeDownCast(caller) == this->AngleModelCenterSelectorWidget &&
           event == vtkSlicerNodeSelectorWidget::NodeSelectedEvent
           && this->GetViewerWidget())
    {
    vtkMRMLModelNode *model = 
      vtkMRMLModelNode::SafeDownCast(this->AngleModelCenterSelectorWidget->GetSelected());
    if (model != NULL  && model->GetDisplayNode() != NULL)
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetModelIDCenter(model->GetID());
      this->Update3DWidget(activeAngleNode);
      }
    else
      {
      // is it a slice node?
      vtkMRMLSliceNode *slice = vtkMRMLSliceNode::SafeDownCast(this->AngleModelCenterSelectorWidget->GetSelected());
      if (slice != NULL && slice->GetID())
        {
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetModelIDCenter(slice->GetID());
        this->Update3DWidget(activeAngleNode);
        }
      else
        {
        // remove the constraint by setting it to null
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetModelIDCenter(NULL);
        this->Update3DWidget(activeAngleNode);
        }
      }
    }
  else if (activeAngleNode == vtkMRMLMeasurementsAngleNode::SafeDownCast(this->AngleSelectorWidget->GetSelected()) &&
           event == vtkCommand::ModifiedEvent)
    {
    vtkDebugMacro("\tmodified event on the angle selected node.\n");
    if (activeAngleNode !=  vtkMRMLMeasurementsAngleNode::SafeDownCast(this->AngleSelectorWidget->GetSelected()))
      {
      // select it first off
      this->SetAngleNodeID(vtkMRMLMeasurementsAngleNode::SafeDownCast(this->AngleSelectorWidget->GetSelected())->GetID());
      }
    vtkDebugMacro("Setting gui from angle node");
    this->UpdateWidget(activeAngleNode);
    return;
    }
  else if (ccbutton && event == vtkKWChangeColorButton::ColorChangedEvent)
    {
    if (ccbutton == this->PointColourButton)
      {
      double *guiRGB = this->PointColourButton->GetColor();
      double *nodeRGB = activeAngleNode->GetPointColour();
      if (nodeRGB == NULL ||
          (fabs(guiRGB[0]-nodeRGB[0]) > 0.001 ||
           fabs(guiRGB[1]-nodeRGB[1]) > 0.001 ||
           fabs(guiRGB[2]-nodeRGB[2]) > 0.001))
        {
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetPointColour(this->PointColourButton->GetColor());
        }
      }
    else if (ccbutton == this->LineColourButton)
      {
      double *guiRGB = this->LineColourButton->GetColor();
      double *nodeRGB = activeAngleNode->GetLineColour();
      if (nodeRGB == NULL ||
          (fabs(guiRGB[0]-nodeRGB[0]) > 0.001 ||
           fabs(guiRGB[1]-nodeRGB[1]) > 0.001 ||
           fabs(guiRGB[2]-nodeRGB[2]) > 0.001))
        {
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetLineColour(this->LineColourButton->GetColor());
        }
      }
    else if (ccbutton == this->TextColourButton)
      {
      double *guiRGB = this->TextColourButton->GetColor();
      double *nodeRGB = activeAngleNode->GetLabelTextColour();
      if (nodeRGB == NULL ||
          (fabs(guiRGB[0]-nodeRGB[0]) > 0.001 ||
           fabs(guiRGB[1]-nodeRGB[1]) > 0.001 ||
           fabs(guiRGB[2]-nodeRGB[2]) > 0.001))
        {
        if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
        activeAngleNode->SetLabelTextColour(this->TextColourButton->GetColor());
        }
      }
    this->Update3DWidget(activeAngleNode);
    }
  else if (entry && event == vtkKWEntry::EntryValueChangedEvent)
    {
    if (entry == this->Position1XEntry ||
        entry == this->Position1YEntry ||
        entry == this->Position1ZEntry)
      {
      double x, y, z;
      double *position = activeAngleNode->GetPosition1();
      // make sure don't undo the changes in the node by getting the gui
      // elements since that would trigger an update to the gui with the old
      // values
      if (entry == this->Position1XEntry)
        {
        x = this->Position1XEntry->GetValueAsDouble();
        }
      else
        {
        // grab the value from the node
        x = position[0];
        }
      if (entry ==  this->Position1YEntry)
        {
        y = this->Position1YEntry->GetValueAsDouble();
        }
      else
        {
        y = position[1];
        }
      if (entry == this->Position1ZEntry)
        {
        z = this->Position1ZEntry->GetValueAsDouble();
        }
      else
        {
        z = position[2];
        }
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetPosition1(x, y, z);
      }
    else if (entry == this->Position2XEntry ||
             entry == this->Position2YEntry ||
             entry == this->Position2ZEntry)
      {
      double x, y, z;
      double *position = activeAngleNode->GetPosition2();
      if (entry == this->Position2XEntry)
        {
        x = this->Position2XEntry->GetValueAsDouble();
        }
      else
        {
        x = position[0];
        }
      if (entry == this->Position2YEntry)
        {
        y = this->Position2YEntry->GetValueAsDouble();
        }
      else
        {
        y = position[1];
        }
      if (entry == this->Position2ZEntry)
        {
        z = this->Position2ZEntry->GetValueAsDouble();
        }
      else
        {
        z = position[2];
        }
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetPosition2(x, y, z);
      }
    else if (entry == this->PositionCenterXEntry ||
             entry == this->PositionCenterYEntry ||
             entry == this->PositionCenterZEntry)
      {
      double x, y, z;
      double *position = activeAngleNode->GetPositionCenter();
      if (entry == this->PositionCenterXEntry)
        {
        x = this->PositionCenterXEntry->GetValueAsDouble();
        }
      else
        {
        x = position[0];
        }
      if (entry == this->PositionCenterYEntry)
        {
        y = this->PositionCenterYEntry->GetValueAsDouble();
        }
      else
        {
        y = position[1];
        }
      if (entry == this->PositionCenterZEntry)
        {
        z = this->PositionCenterZEntry->GetValueAsDouble();
        }
      else
        {
        z = position[2];
        }
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetPositionCenter(x, y, z);
      }
    else if (entry == this->LabelFormatEntry->GetWidget())
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetLabelFormat(this->LabelFormatEntry->GetWidget()->GetValue());
      }
    else if (entry == this->LabelScaleEntry->GetWidget())
      {
      double scale = this->LabelScaleEntry->GetWidget()->GetValueAsDouble();
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetLabelScale(scale, scale, scale);
      }
    else if (entry == this->ResolutionEntry->GetWidget())
      {
      if (this->MRMLScene) { this->MRMLScene->SaveStateForUndo(activeAngleNode); }
      activeAngleNode->SetResolution(this->ResolutionEntry->GetWidget()->GetValueAsInt());
      }
    this->Update3DWidget(activeAngleNode);
    }
}


//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::ProcessMRMLEvents ( vtkObject *caller,
                                              unsigned long event, void *callData )
{
  
 vtkMRMLScene *callScene = vtkMRMLScene::SafeDownCast(caller);

 // the scene was closed, don't get node removed events so clear up here
  if (callScene != NULL &&
      event == vtkMRMLScene::SceneCloseEvent)
    {
    vtkDebugMacro("ProcessMRMLEvents: got a scene close event");
    // the lists are already gone from the scene, so need to clear out all the
    // widget properties, can't call remove with a node
    this->Update3DWidgetsFromMRML();
    return;
    }

  // first check to see if there was a angle list node deleted
  if (callScene != NULL &&
      callScene == this->MRMLScene &&
      event == vtkMRMLScene::NodeRemovedEvent)
    {
    vtkDebugMacro("ProcessMRMLEvents: got a node deleted event on scene");
    // check to see if it was the current node that was deleted
    if (callData != NULL)
      {
      vtkMRMLMeasurementsAngleNode *delNode = reinterpret_cast<vtkMRMLMeasurementsAngleNode*>(callData);
      if (delNode != NULL &&
          delNode->IsA("vtkMRMLMeasurementsAngleNode"))
        {
        vtkDebugMacro("A angle node got deleted " << (delNode->GetID() == NULL ? "null" : delNode->GetID()));
        this->RemoveAngleWidget(delNode);
        }
      }
    }

  vtkMRMLMeasurementsAngleNode *node = vtkMRMLMeasurementsAngleNode::SafeDownCast(caller);
  vtkMRMLMeasurementsAngleNode *activeAngleNode = (vtkMRMLMeasurementsAngleNode *)this->MRMLScene->GetNodeByID(this->GetAngleNodeID());

  // check for a node added event
  if (callScene != NULL &&
      callScene == this->MRMLScene &&
      callData != NULL &&
      event == vtkMRMLScene::NodeAddedEvent)
    {
    vtkDebugMacro("ProcessMRMLEvents: got a node added event on scene");
    // check to see if it was a angle node    
    vtkMRMLMeasurementsAngleNode *addNode = reinterpret_cast<vtkMRMLMeasurementsAngleNode*>(callData);
    if (addNode != NULL &&
        addNode->IsA("vtkMRMLMeasurementsAngleNode"))
      {
      vtkDebugMacro("Got a node added event with a angle node " << addNode->GetID());
      // is it currently the active one?
      if (addNode == activeAngleNode)
        {
        vtkDebugMacro("Calling Update widget to set up the ui since this is the active one");
        this->UpdateWidget(addNode);
        }
      // if it's not the current one, just update the 3d widget
      vtkDebugMacro("Calling Update 3D widget to set up a new angle widget");
      this->Update3DWidget(addNode);
      // for now, since missing some of the add calls when open a scene, make sure we're current with the scene
      this->Update3DWidgetsFromMRML();
      return;
      }
    }

  else if (node == activeAngleNode)
    {
    if (event == vtkCommand::ModifiedEvent || event == vtkMRMLScene::NodeAddedEvent || event == vtkMRMLScene::NodeRemovedEvent)
      {
      vtkDebugMacro("Modified or node added or removed event on the angle node.\n");
      if (node == NULL)
        {
        vtkDebugMacro("\tBUT: the node is null\n");
        // check to see if the id used to get the node is not null, if it's
        // a valid string, means that the node was deleted
        if (this->GetAngleNodeID() != NULL)
          {
          this->SetAngleNodeID(NULL);
          }
        return;
        }
      vtkDebugMacro("ProcessMRMLEvents: \t\tUpdating the GUI\n");
      // update the gui
      UpdateWidget(activeAngleNode);
      return;
      }
    } // end of events on the active angle node
  else if (node != NULL &&
           event == vtkCommand::ModifiedEvent)
    {
    // it's a modified event on a angle node that's not being displayed in the
    // 2d gui, so update the 3d widget
    this->Update3DWidget(node);
    }  
  else if (node != NULL &&
      event == vtkMRMLTransformableNode::TransformModifiedEvent)
    {
    vtkDebugMacro("Got transform modified event on node " << node->GetID());
    this->Update3DWidget(node);
    }
}


//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::UpdateWidget(vtkMRMLMeasurementsAngleNode *activeAngleNode)
{ 

  vtkDebugMacro("UpdateWidget: active angle node is " << (activeAngleNode == NULL ? "null" : activeAngleNode->GetName()));
  
  // if the passed node is null, clear out the widget
  if (activeAngleNode == NULL)
    {
    // don't need to do anything yet, especially don't set the node selector to
    // null, as it causes a crash
    vtkDebugMacro("UpdateWidget: The passed in node is null, returning.");
    return;
    }
  
  if ( this->AngleSelectorWidget->GetSelected() == NULL )
    {
    vtkDebugMacro("Null selected angle, selecting it and returning");
    this->AngleSelectorWidget->SetSelected(activeAngleNode);    
    return;
    }

  if (activeAngleNode && this->AngleSelectorWidget->GetSelected() &&
        strcmp(activeAngleNode->GetName(),
               this->AngleSelectorWidget->GetSelected()->GetName()) != 0)
      {
      vtkDebugMacro("UpdateWidget: input angle " << activeAngleNode->GetName() << " doesn't match selector widget value: " << this->AngleSelectorWidget->GetSelected()->GetName());
      this->AngleSelectorWidget->SetSelected(activeAngleNode);
      vtkDebugMacro("... returning, hoping for a invoke event");
      return;
      }

  vtkDebugMacro("UpdateWidget: updating the gui and 3d elements");
  // first update the GUI, then update the 3d elements
  // visibility
  this->VisibilityButton->GetWidget()->SetSelectedState(activeAngleNode->GetVisibility());
  this->Ray1VisibilityButton->GetWidget()->SetSelectedState(activeAngleNode->GetRay1Visibility());
  this->Ray2VisibilityButton->GetWidget()->SetSelectedState(activeAngleNode->GetRay2Visibility());
  this->ArcVisibilityButton->GetWidget()->SetSelectedState(activeAngleNode->GetArcVisibility());

  // end point positions
  double *position = activeAngleNode->GetPosition1();
  if (position)
    {
    this->Position1XEntry->SetValueAsDouble(position[0]);
    this->Position1YEntry->SetValueAsDouble(position[1]);
    this->Position1ZEntry->SetValueAsDouble(position[2]);
    }
  position = activeAngleNode->GetPosition2();
  if (position)
    {
    this->Position2XEntry->SetValueAsDouble(position[0]);
    this->Position2YEntry->SetValueAsDouble(position[1]);
    this->Position2ZEntry->SetValueAsDouble(position[2]);
    }
  position = activeAngleNode->GetPositionCenter();
  if (position)
    {
    this->PositionCenterXEntry->SetValueAsDouble(position[0]);
    this->PositionCenterYEntry->SetValueAsDouble(position[1]);
    this->PositionCenterZEntry->SetValueAsDouble(position[2]);
    }

  // constraints
  const char *modelID1 = activeAngleNode->GetModelID1();
  if (modelID1)
    {
    // get the node
    vtkMRMLNode *model = this->GetMRMLScene()->GetNodeByID(modelID1);
    if (model)
      {
      this->AngleModel1SelectorWidget->SetSelected(model);
      }
    }

  const char *modelID2 = activeAngleNode->GetModelID2();
  if (modelID2)
    {
    // get the second  node 
    vtkMRMLNode *model = this->GetMRMLScene()->GetNodeByID(modelID2);
    if (model)
      {
      this->AngleModel2SelectorWidget->SetSelected(model);
      }
    }

  const char *modelIDCenter = activeAngleNode->GetModelIDCenter();
  if (modelIDCenter)
    {
    // get the second  node 
    vtkMRMLNode *model = this->GetMRMLScene()->GetNodeByID(modelIDCenter);
    if (model)
      {
      this->AngleModelCenterSelectorWidget->SetSelected(model);
      }
    }
  
  // end point colour
  double *rgb = this->PointColourButton->GetColor();
  double *rgb1 = activeAngleNode->GetPointColour();
  if (fabs(rgb[0]-rgb1[0]) > 0.001 ||
      fabs(rgb[1]-rgb1[1]) > 0.001 ||
      fabs(rgb[2]-rgb1[2]) > 0.001)
    {
    this->PointColourButton->SetColor(activeAngleNode->GetPointColour());
    }
  
  // line colour
  rgb = this->LineColourButton->GetColor();
  rgb1 = activeAngleNode->GetLineColour();
  if (fabs(rgb[0]-rgb1[0]) > 0.001 ||
      fabs(rgb[1]-rgb1[1]) > 0.001 ||
      fabs(rgb[2]-rgb1[2]) > 0.001)
    {
    this->LineColourButton->SetColor(activeAngleNode->GetLineColour());
    }

 

  // angle annotation
  rgb = this->TextColourButton->GetColor();
  rgb1 = activeAngleNode->GetLabelTextColour();
  if (fabs(rgb[0]-rgb1[0]) > 0.001 ||
      fabs(rgb[1]-rgb1[1]) > 0.001 ||
      fabs(rgb[2]-rgb1[2]) > 0.001)
    {
    this->TextColourButton->SetColor(activeAngleNode->GetLabelTextColour());
    }
  this->LabelVisibilityButton->GetWidget()->SetSelectedState(activeAngleNode->GetLabelVisibility());
  this->LabelFormatEntry->GetWidget()->SetValue(activeAngleNode->GetLabelFormat());
  double *scale = activeAngleNode->GetLabelScale();
  if (scale)
    {
    this->LabelScaleEntry->GetWidget()->SetValueAsDouble(scale[0]);
    }

  // resolution
//  this->ResolutionEntry->GetWidget()->SetValueAsInt(activeAngleNode->GetResolution());

  this->Update3DWidget(activeAngleNode);
   
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::Update3DWidget(vtkMRMLMeasurementsAngleNode *activeAngleNode)
{
  if (activeAngleNode == NULL)
    {
    vtkDebugMacro("Update3DWidget: passed in angle node is null, returning");
    return;
    }
  if (this->Updating3DWidget)
    {
    vtkDebugMacro("Already updating 3d widget");
    return;
    }
  vtkMeasurementsAngleWidgetClass *angleWidget = this->GetAngleWidget(activeAngleNode->GetID());
  if (!angleWidget)
    {
    vtkDebugMacro("No angle widget found for anglenode " << activeAngleNode->GetID() << ", have " << this->AngleWidgets.size() << " widgets, adding one for this one");
    this->AddAngleWidget(activeAngleNode);
    angleWidget = this->GetAngleWidget(activeAngleNode->GetID());
    if (!angleWidget)
      {
      vtkErrorMacro("Error adding a new angle widget for angle node " << activeAngleNode->GetID());
      this->Updating3DWidget = 0;
      return;
      }
    }
  if (angleWidget->GetWidget() == NULL)
    {
    vtkErrorMacro("Update3D widget: angle widget is null");
    return;
    }
  if (angleWidget->GetRepresentation() == NULL)
    {
    vtkErrorMacro("Update3D widget: angle representation is null");
    return;
    }
  this->Updating3DWidget = 1;

  vtkDebugMacro("Updating 3d widget from " << activeAngleNode->GetID());
  
  // visibility
  if ( activeAngleNode->GetVisibility() )
    {
    if (angleWidget->GetWidget()->GetInteractor() == NULL &&
        this->GetViewerWidget())
      {
      angleWidget->GetWidget()->SetInteractor(this->GetViewerWidget()->GetMainViewer()->GetRenderWindowInteractor());
      double *p1 = activeAngleNode->GetPosition1();
      double *p2 = activeAngleNode->GetPosition2();
      double *pCenter = activeAngleNode->GetPositionCenter();
      angleWidget->GetRepresentation()->SetPoint1WorldPosition(p1);
      angleWidget->GetRepresentation()->SetPoint2WorldPosition(p2);
      angleWidget->GetRepresentation()->SetCenterWorldPosition(pCenter);
      // at this point the angle widget is still waiting for three clicks to
      // start and define the angle, so fool it into thinking that the three
      // clicks have happened
      int *max;
      max = angleWidget->GetWidget()->GetInteractor()->GetSize();
      double x, y;
      x = (double)max[0];
      y = (double)max[1];
      angleWidget->GetWidget()->GetInteractor()->SetEventPositionFlipY(x*0.25,y*0.25);
      angleWidget->GetWidget()->On();
      angleWidget->GetWidget()->GetInteractor()->InvokeEvent(vtkCommand::LeftButtonPressEvent);
      angleWidget->GetWidget()->GetInteractor()->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
      angleWidget->GetWidget()->GetInteractor()->SetEventPositionFlipY(x*0.5,y*0.5);
      angleWidget->GetWidget()->GetInteractor()->InvokeEvent(vtkCommand::LeftButtonPressEvent);
      angleWidget->GetWidget()->GetInteractor()->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
      angleWidget->GetWidget()->GetInteractor()->SetEventPositionFlipY(x*0.75,y*0.25);
      angleWidget->GetWidget()->GetInteractor()->InvokeEvent(vtkCommand::LeftButtonPressEvent);
      angleWidget->GetWidget()->GetInteractor()->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
      
      }
    vtkDebugMacro("UpdateWidget: angle widget on");
    angleWidget->GetWidget()->On();
    }
  else
    {
    vtkDebugMacro("UpdateWidget: angle widget off");
    angleWidget->GetWidget()->Off();
    }

  if (angleWidget->GetRepresentation())
    {
    // end point colour
    double *rgb1 = activeAngleNode->GetPointColour();
    angleWidget->GetHandleRepresentation()->GetProperty()->SetColor(rgb1[0], rgb1[1], rgb1[2]);
    vtkPointHandleRepresentation3D *rep = vtkPointHandleRepresentation3D::SafeDownCast(angleWidget->GetRepresentation()->GetPoint1Representation());
    if (rep)
      {
      rep->GetProperty()->SetColor(rgb1[0], rgb1[1], rgb1[2]);
      }
    rep = vtkPointHandleRepresentation3D::SafeDownCast(angleWidget->GetRepresentation()->GetPoint2Representation());
    if (rep)
      {
      rep->GetProperty()->SetColor(rgb1[0], rgb1[1], rgb1[2]);
      }
    rep = vtkPointHandleRepresentation3D::SafeDownCast(angleWidget->GetRepresentation()->GetCenterRepresentation());
    if (rep)
      {
      rep->GetProperty()->SetColor(rgb1[0], rgb1[1], rgb1[2]);
      }
    // line colour
    rgb1 = activeAngleNode->GetLineColour();
    angleWidget->GetRepresentation()->GetRay1()->GetProperty()->SetColor(rgb1[0], rgb1[1], rgb1[2]);
    angleWidget->GetRepresentation()->GetRay2()->GetProperty()->SetColor(rgb1[0], rgb1[1], rgb1[2]);
    //angleWidget->GetRepresentation()->GetArc()->GetProperty()->SetColor(rgb1[0], rgb1[1], rgb1[2]);

    // text colour
    rgb1 = activeAngleNode->GetLabelTextColour();
    //angleWidget->GetRepresentation()->GetTextActor()->GetProperty()->SetColor(rgb1[0], rgb1[1], rgb1[2]);

    // position
    // get any transform on the node
    vtkMRMLTransformNode* tnode = activeAngleNode->GetParentTransformNode();
    vtkMatrix4x4* transformToWorld = vtkMatrix4x4::New();
    transformToWorld->Identity();
    if (tnode != NULL && tnode->IsLinear())
      {
      vtkMRMLLinearTransformNode *lnode = vtkMRMLLinearTransformNode::SafeDownCast(tnode);
      lnode->GetMatrixTransformToWorld(transformToWorld);
      }
    double *p = activeAngleNode->GetPosition1();
    if (p)
      {
      // convert by the parent transform
      double xyzw[4];
      xyzw[0] = p[0];
      xyzw[1] = p[1];
      xyzw[2] = p[2];
      xyzw[3] = 1.0;
      double worldxyz[4], *worldp = &worldxyz[0];
      transformToWorld->MultiplyPoint(xyzw, worldp);
      angleWidget->GetRepresentation()->SetPoint1WorldPosition(worldp);
      }
    p =  activeAngleNode->GetPosition2();
    if (p)
      {
      // convert by the parent transform
      double xyzw[4];
      xyzw[0] = p[0];
      xyzw[1] = p[1];
      xyzw[2] = p[2];
      xyzw[3] = 1.0;
      double worldxyz[4], *worldp = &worldxyz[0];
      transformToWorld->MultiplyPoint(xyzw, worldp);
      angleWidget->GetRepresentation()->SetPoint2WorldPosition(worldp);
      }
    p =  activeAngleNode->GetPositionCenter();
    if (p)
      {
      // convert by the parent transform
      double xyzw[4];
      xyzw[0] = p[0];
      xyzw[1] = p[1];
      xyzw[2] = p[2];
      xyzw[3] = 1.0;
      double worldxyz[4], *worldp = &worldxyz[0];
      transformToWorld->MultiplyPoint(xyzw, worldp);
      angleWidget->GetRepresentation()->SetCenterWorldPosition(worldp);
      }
    tnode = NULL;
    transformToWorld->Delete();
    transformToWorld = NULL;

    // sub component visibility
    angleWidget->GetRepresentation()->SetRay1Visibility(activeAngleNode->GetRay1Visibility());
    angleWidget->GetRepresentation()->SetRay2Visibility(activeAngleNode->GetRay2Visibility());
    angleWidget->GetRepresentation()->SetArcVisibility(activeAngleNode->GetArcVisibility());
    
    // angle annotation
    //angleWidget->GetRepresentation()->SetLabelVisibility(activeAngleNode->GetLabelVisibility());
    angleWidget->GetRepresentation()->SetLabelFormat(activeAngleNode->GetLabelFormat());
    double *scale = activeAngleNode->GetLabelScale();
    if (scale)
      {
      angleWidget->GetRepresentation()->SetTextActorScale(scale);
      }
    // resolution
    //angleWidget->GetRepresentation()->SetResolution(activeAngleNode->GetResolution());
    }

  // first point constraint
  if (activeAngleNode->GetModelID1())
    {
    // get the model node
    vtkMRMLModelNode *model = 
      vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(activeAngleNode->GetModelID1()));
    vtkMRMLSliceNode *slice =
      vtkMRMLSliceNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(activeAngleNode->GetModelID1()));
    // is it a slice node?
    if (slice)
      {
      // get the model node associated with it
      vtkDebugMacro("Update3DWidget: Have a slice node, id = " << slice->GetID());
      // get the model node associated with it
      std::string modelName = std::string(slice->GetName()) + std::string(" Volume Slice");
      vtkCollection *modelCollection = this->GetMRMLScene()->GetNodesByName(modelName.c_str());
      if (modelCollection &&
          modelCollection->GetNumberOfItems() > 0)
        {
        model = vtkMRMLModelNode::SafeDownCast(modelCollection->GetItemAsObject(0));
        }
      }
    // is it a valid model?
    if (model &&
        model->GetDisplayNode())
      {
      if (model->GetDisplayNode()->GetVisibility() == 0)
        {
        if (slice)
          {
          vtkWarningMacro("The " <<  slice->GetName() << " slice is not visible, you won't be able to move the end point.");
          }
        else
          {
          vtkWarningMacro("The " <<  model->GetName() << " model is not visible, you won't be able to move the end point");
          }
        }
      vtkProp *prop = vtkProp::SafeDownCast(this->GetViewerWidget()->GetActorByID(model->GetDisplayNode()->GetID()));
      // is it already set to constrain the point placer?
      if (prop &&
          !angleWidget->GetModel1PointPlacer()->HasProp(prop))
        {
        // clear out any others
        angleWidget->GetModel1PointPlacer()->RemoveAllProps();
        // add this one
        angleWidget->GetModel1PointPlacer()->AddProp(prop);
        angleWidget->GetRepresentation()->GetPoint1Representation()->ConstrainedOff();
        angleWidget->GetRepresentation()->GetPoint1Representation()->SetPointPlacer(angleWidget->GetModel1PointPlacer());
        }
      }
    else
      {
      angleWidget->GetModel1PointPlacer()->RemoveAllProps();
      angleWidget->GetRepresentation()->GetPoint1Representation()->SetPointPlacer(NULL);
      }
    }
  else
    {
    // make sure it's not constrained
    angleWidget->GetModel1PointPlacer()->RemoveAllProps();
    angleWidget->GetRepresentation()->GetPoint1Representation()->SetPointPlacer(NULL);
    }

  // second point constraint
  if (activeAngleNode->GetModelID2())
    {
    // get the model node
    vtkMRMLModelNode *model = 
      vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(activeAngleNode->GetModelID2()));
     vtkMRMLSliceNode *slice =
      vtkMRMLSliceNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(activeAngleNode->GetModelID2()));
    // is it a slice node?
    if (slice)
      {
      // get the model node associated with it
      vtkDebugMacro("Update3DWidget: Have a slice node, id = " << slice->GetID());
      // get the model node associated with it
      std::string modelName = std::string(slice->GetName()) + std::string(" Volume Slice");
      vtkCollection *modelCollection = this->GetMRMLScene()->GetNodesByName(modelName.c_str());
      if (modelCollection &&
          modelCollection->GetNumberOfItems() > 0)
        {
        model = vtkMRMLModelNode::SafeDownCast(modelCollection->GetItemAsObject(0));
        }
      }
    // is it a valid model?
    if (model &&
        model->GetDisplayNode())
      {
      vtkProp *prop = vtkProp::SafeDownCast(this->GetViewerWidget()->GetActorByID(model->GetDisplayNode()->GetID()));
      // is it already set to constrain the point placer?
      if (prop &&
          !angleWidget->GetModel2PointPlacer()->HasProp(prop))
        {
        // clear out any others
        angleWidget->GetModel2PointPlacer()->RemoveAllProps();
        // add this one
        angleWidget->GetModel2PointPlacer()->AddProp(prop);
        angleWidget->GetRepresentation()->GetPoint2Representation()->ConstrainedOff();
        angleWidget->GetRepresentation()->GetPoint2Representation()->SetPointPlacer(angleWidget->GetModel2PointPlacer());
        /*
        // check if need to snap to it
        // TODO: figure out why not snapping
        double pos[3];
        angleWidget->GetRepresentation()->GetPoint2WorldPosition(pos);
        if (!angleWidget->GetRepresentation()->GetPoint2Representation()->GetPointPlacer()->ValidateWorldPosition(pos))
          {
          if (model->GetPolyData())
            {
            model->GetPolyData()->GetPoint(0, pos);
            vtkDebugMacro("Snapping point 2 to " << pos[0] << ", " << pos[1] << ", " << pos[2]);
            angleWidget->GetRepresentation()->SetPoint2WorldPosition(pos);
            }
          }
        */
        }
      }
    else
      {
      angleWidget->GetModel2PointPlacer()->RemoveAllProps();
//      angleWidget->GetHandleRepresentation->ConstrainedOn();
      angleWidget->GetRepresentation()->GetPoint2Representation()->SetPointPlacer(NULL);
      }
    }
  else
    {
    // make sure it's not constrained
    angleWidget->GetModel2PointPlacer()->RemoveAllProps();
    angleWidget->GetRepresentation()->GetPoint2Representation()->SetPointPlacer(NULL);
    }

  // center point constraint
  if (activeAngleNode->GetModelIDCenter())
    {
    // get the model node
    vtkMRMLModelNode *model = 
      vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(activeAngleNode->GetModelIDCenter()));
     vtkMRMLSliceNode *slice =
      vtkMRMLSliceNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(activeAngleNode->GetModelIDCenter()));
    // is it a slice node?
    if (slice)
      {
      // get the model node associated with it
      vtkDebugMacro("Update3DWidget: Have a slice node, id = " << slice->GetID());
      // get the model node associated with it
      std::string modelName = std::string(slice->GetName()) + std::string(" Volume Slice");
      vtkCollection *modelCollection = this->GetMRMLScene()->GetNodesByName(modelName.c_str());
      if (modelCollection &&
          modelCollection->GetNumberOfItems() > 0)
        {
        model = vtkMRMLModelNode::SafeDownCast(modelCollection->GetItemAsObject(0));
        }
      }
    // is it a valid model?
    if (model &&
        model->GetDisplayNode())
      {
      vtkProp *prop = vtkProp::SafeDownCast(this->GetViewerWidget()->GetActorByID(model->GetDisplayNode()->GetID()));
      // is it already set to constrain the point placer?
      if (prop &&
          !angleWidget->GetModelCenterPointPlacer()->HasProp(prop))
        {
        // clear out any others
        angleWidget->GetModelCenterPointPlacer()->RemoveAllProps();
        // add this one
        angleWidget->GetModelCenterPointPlacer()->AddProp(prop);
        angleWidget->GetRepresentation()->GetCenterRepresentation()->ConstrainedOff();
        angleWidget->GetRepresentation()->GetCenterRepresentation()->SetPointPlacer(angleWidget->GetModelCenterPointPlacer());
        /*
        // check if need to snap to it
        // TODO: figure out why not snapping
        double pos[3];
        angleWidget->GetRepresentation()->GetCenterWorldPosition(pos);
        if (!angleWidget->GetRepresentation()->GetCenterRepresentation()->GetPointPlacer()->ValidateWorldPosition(pos))
          {
          if (model->GetPolyData())
            {
            model->GetPolyData()->GetPoint(0, pos);
            vtkDebugMacro("Snapping point Center to " << pos[0] << ", " << pos[1] << ", " << pos[2]);
            angleWidget->GetRepresentation()->SetCenterWorldPosition(pos);
            }
          }
        */
        }
      }
    else
      {
      angleWidget->GetModelCenterPointPlacer()->RemoveAllProps();
//      angleWidget->GetHandleRepresentation->ConstrainedOn();
      angleWidget->GetRepresentation()->GetCenterRepresentation()->SetPointPlacer(NULL);
      }
    }
  else
    {
    // make sure it's not constrained
    angleWidget->GetModelCenterPointPlacer()->RemoveAllProps();
    angleWidget->GetRepresentation()->GetCenterRepresentation()->SetPointPlacer(NULL);
    }

  
  // set up call back
  // temp: remove observers
  angleWidget->GetWidget()->RemoveObservers(vtkCommand::InteractionEvent);
  angleWidget->GetWidget()->RemoveObservers(vtkCommand::StartInteractionEvent);

  // now add call back
  vtkMeasurementsAngleWidgetCallback *myCallback = vtkMeasurementsAngleWidgetCallback::New();
//  std::string angleID = std::string(activeAngleNode->GetID());
//  myCallback->AngleID = angleID;
  myCallback->AngleNode = activeAngleNode;
//  myCallback->Representation = angleWidget->GetRepresentation();
  angleWidget->GetWidget()->AddObserver(vtkCommand::InteractionEvent,myCallback);
  angleWidget->GetWidget()->AddObserver(vtkCommand::StartInteractionEvent, myCallback);
  myCallback->Delete();

  // request a render
  if (this->ViewerWidget)
    {
    this->ViewerWidget->RequestRender();
    }
  // reset the flag
  this->Updating3DWidget = 0;
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::AddMRMLObservers ( )
{
  // the widget as a whole needs to keep track of angle nodes in the scene
  if (this->MRMLScene)
    {
    vtkDebugMacro("AddMRMLObservers: watching for node removed, added, scene close events on the scene");
    if (this->MRMLScene->HasObserver(vtkMRMLScene::NodeRemovedEvent, (vtkCommand *)this->MRMLCallbackCommand) != 1)
      {
      this->MRMLScene->AddObserver(vtkMRMLScene::NodeRemovedEvent, (vtkCommand *)this->MRMLCallbackCommand);
      }
    if (this->MRMLScene->HasObserver(vtkMRMLScene::NodeAddedEvent, (vtkCommand *)this->MRMLCallbackCommand) != 1)
      {
      this->MRMLScene->AddObserver(vtkMRMLScene::NodeAddedEvent, (vtkCommand *)this->MRMLCallbackCommand);
      }
    if (this->MRMLScene->HasObserver(vtkMRMLScene::SceneCloseEvent, (vtkCommand *)this->MRMLCallbackCommand) != 1)
      {
      this->MRMLScene->AddObserver(vtkMRMLScene::SceneCloseEvent, (vtkCommand *)this->MRMLCallbackCommand);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::RemoveMRMLObservers ( )
{
  // remove observers on the angle nodes
  int nnodes = this->MRMLScene->GetNumberOfNodesByClass("vtkMRMLMeasurementsAngleNode");
  //vtkIntArray *events = vtkIntArray::New();
  //events->InsertNextValue(vtkCommand::ModifiedEvent);
//  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
//  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  //events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  for (int n=0; n<nnodes; n++)
    {
    vtkMRMLMeasurementsAngleNode *angleNode = vtkMRMLMeasurementsAngleNode::SafeDownCast(this->MRMLScene->GetNthNodeByClass(n, "vtkMRMLMeasurementsAngleNode"));
    //vtkSetAndObserveMRMLNodeEventsMacro(angleNode, NULL, events);
    if (angleNode->HasObserver(vtkMRMLTransformableNode::TransformModifiedEvent, (vtkCommand *)this->MRMLCallbackCommand))
      {
      angleNode->RemoveObservers(vtkMRMLTransformableNode::TransformModifiedEvent, (vtkCommand *)this->MRMLCallbackCommand);
      }
    if (angleNode->HasObserver(vtkCommand::ModifiedEvent, (vtkCommand *)this->MRMLCallbackCommand))
      {
      angleNode->RemoveObservers(vtkCommand::ModifiedEvent, (vtkCommand *)this->MRMLCallbackCommand);
      }
    angleNode = NULL;
    }
  //events->Delete();

  if (this->MRMLScene)
    {
    vtkDebugMacro("RemoveMRMLObservers: stopping watching for node removed, added, scene close events on the scene");
    this->MRMLScene->RemoveObservers(vtkMRMLScene::NodeRemovedEvent, (vtkCommand *)this->MRMLCallbackCommand);
    this->MRMLScene->RemoveObservers(vtkMRMLScene::NodeAddedEvent, (vtkCommand *)this->MRMLCallbackCommand);
    this->MRMLScene->RemoveObservers(vtkMRMLScene::SceneCloseEvent, (vtkCommand *)this->MRMLCallbackCommand);
    }
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::AddWidgetObservers()
{
  if (this->AllVisibilityMenuButton)
    {
    this->AllVisibilityMenuButton->GetMenu()->AddObserver ( vtkKWMenu::MenuItemInvokedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->VisibilityButton)
    {
    this->VisibilityButton->GetWidget()->AddObserver(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Ray1VisibilityButton)
    {
    this->Ray1VisibilityButton->GetWidget()->AddObserver(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Ray2VisibilityButton)
    {
    this->Ray2VisibilityButton->GetWidget()->AddObserver(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->ArcVisibilityButton)
    {
    this->ArcVisibilityButton->GetWidget()->AddObserver(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if ( this->PointColourButton)
    {
    this->PointColourButton->AddObserver(vtkKWChangeColorButton::ColorChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if ( this->LineColourButton)
    {
    this->LineColourButton->AddObserver(vtkKWChangeColorButton::ColorChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if ( this->TextColourButton)
    {
    this->TextColourButton->AddObserver(vtkKWChangeColorButton::ColorChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->AngleSelectorWidget)
    {
    this->AngleSelectorWidget->AddObserver (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->AngleModel1SelectorWidget)
    {
    this->AngleModel1SelectorWidget->AddObserver (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->AngleModel2SelectorWidget)
    {
    this->AngleModel2SelectorWidget->AddObserver (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->AngleModelCenterSelectorWidget)
    {
    this->AngleModelCenterSelectorWidget->AddObserver (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->LabelVisibilityButton)
    {
    this->LabelVisibilityButton->GetWidget()->AddObserver(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->Position1XEntry)
    {
    this->Position1XEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Position1YEntry)
    {
    this->Position1YEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Position1ZEntry)
    {
    this->Position1ZEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->Position2XEntry)
    {
    this->Position2XEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Position2YEntry)
    {
    this->Position2YEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Position2ZEntry)
    {
    this->Position2ZEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->PositionCenterXEntry)
    {
    this->PositionCenterXEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->PositionCenterYEntry)
    {
    this->PositionCenterYEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->PositionCenterZEntry)
    {
    this->PositionCenterZEntry->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->LabelFormatEntry)
    {
    this->LabelFormatEntry->GetWidget()->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->LabelScaleEntry)
    {
    this->LabelScaleEntry->GetWidget()->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->ResolutionEntry)
    {
    this->ResolutionEntry->GetWidget()->AddObserver(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

//  this->AddMRMLObservers();
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::RemoveWidgetObservers ( )
{
  if (this->AllVisibilityMenuButton)
    {
    this->AllVisibilityMenuButton->GetMenu()->RemoveObservers ( vtkKWMenu::MenuItemInvokedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->VisibilityButton)
    {
    this->VisibilityButton->GetWidget()->RemoveObservers(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Ray1VisibilityButton)
    {
    this->Ray1VisibilityButton->GetWidget()->RemoveObservers(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Ray2VisibilityButton)
    {
    this->Ray2VisibilityButton->GetWidget()->RemoveObservers(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->ArcVisibilityButton)
    {
    this->ArcVisibilityButton->GetWidget()->RemoveObservers(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if ( this->PointColourButton)
    {
    this->PointColourButton->RemoveObservers(vtkKWChangeColorButton::ColorChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if ( this->LineColourButton)
    {
    this->LineColourButton->RemoveObservers(vtkKWChangeColorButton::ColorChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if ( this->TextColourButton)
    {
    this->TextColourButton->RemoveObservers(vtkKWChangeColorButton::ColorChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->AngleSelectorWidget)
    {
    this->AngleSelectorWidget->RemoveObservers (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->AngleModel1SelectorWidget)
    {
    this->AngleModel1SelectorWidget->RemoveObservers (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->AngleModel2SelectorWidget)
    {
    this->AngleModel2SelectorWidget->RemoveObservers (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->AngleModelCenterSelectorWidget)
    {
    this->AngleModelCenterSelectorWidget->RemoveObservers (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->LabelVisibilityButton)
    {
    this->LabelVisibilityButton->GetWidget()->RemoveObservers(vtkKWCheckButton::SelectedStateChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->Position1XEntry)
    {
    this->Position1XEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Position1YEntry)
    {
    this->Position1YEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Position1ZEntry)
    {
    this->Position1ZEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->Position2XEntry)
    {
    this->Position2XEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Position2YEntry)
    {
    this->Position2YEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->Position2ZEntry)
    {
    this->Position2ZEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->PositionCenterXEntry)
    {
    this->PositionCenterXEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->PositionCenterYEntry)
    {
    this->PositionCenterYEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->PositionCenterZEntry)
    {
    this->PositionCenterZEntry->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

  if (this->LabelFormatEntry)
    {
    this->LabelFormatEntry->GetWidget()->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->LabelScaleEntry)
    {
    this->LabelScaleEntry->GetWidget()->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }
  if (this->ResolutionEntry)
    {
    this->ResolutionEntry->GetWidget()->RemoveObservers(vtkKWEntry::EntryValueChangedEvent, (vtkCommand *)this->GUICallbackCommand );
    }

//  this->RemoveMRMLObservers();
}


//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::CreateWidget ( )
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }
  
  // Call the superclass to create the whole widget
  
  this->Superclass::CreateWidget();

  // to get at some top level icons
  vtkSlicerApplication *app = vtkSlicerApplication::SafeDownCast (this->GetApplication() );
  if ( !app )
    {
    vtkErrorMacro ( "CreateWidget: got Null SlicerApplication" );
    return;
    }
  vtkSlicerApplicationGUI *appGUI = app->GetApplicationGUI();
  if ( !appGUI )
    {
    vtkErrorMacro ( "CreateWidget: got Null SlicerApplicationGUI" );
    return;
    }
  
  // ---
  // GLOBAL CONTROLS FRAME
  vtkSlicerModuleCollapsibleFrame *controlAllFrame = vtkSlicerModuleCollapsibleFrame::New();
  controlAllFrame->SetParent ( this->GetParent() );
  controlAllFrame->Create();
  controlAllFrame->SetLabelText ("Modify All Angle Nodes" );
  controlAllFrame->ExpandFrame();
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2 -in %s",
                controlAllFrame->GetWidgetName(),
                this->GetParent()->GetWidgetName());

  //---
  //--- create all visibility menu button and set up menu
  //---
  int index = 0;
  this->AllVisibilityMenuButton = vtkKWMenuButton::New();
  this->AllVisibilityMenuButton->SetParent ( controlAllFrame->GetFrame() );
  this->AllVisibilityMenuButton->Create();
  this->AllVisibilityMenuButton->SetBorderWidth(0);
  this->AllVisibilityMenuButton->SetReliefToFlat();
  this->AllVisibilityMenuButton->IndicatorVisibilityOff();
  this->AllVisibilityMenuButton->SetImageToIcon ( appGUI->GetSlicerFoundationIcons()->GetSlicerVisibleOrInvisibleIcon() );
  this->AllVisibilityMenuButton->SetBalloonHelpString ( "Set visibility on all angle nodes." );
  this->AllVisibilityMenuButton->GetMenu()->AddRadioButton ( "All Angles Visible");
  index = this->AllVisibilityMenuButton->GetMenu()->GetIndexOfItem ("All Angles Visible");
  this->AllVisibilityMenuButton->GetMenu()->SetItemImageToIcon (index, appGUI->GetSlicerFoundationIcons()->GetSlicerVisibleIcon()  );
  this->AllVisibilityMenuButton->GetMenu()->SetItemCompoundModeToLeft ( index );
  this->AllVisibilityMenuButton->GetMenu()->SetItemIndicatorVisibility ( index, 0);
  this->AllVisibilityMenuButton->GetMenu()->AddRadioButton ( "All Angles Invisible");
  index = this->AllVisibilityMenuButton->GetMenu()->GetIndexOfItem ("All Angles Invisible");
  this->AllVisibilityMenuButton->GetMenu()->SetItemImageToIcon (index, appGUI->GetSlicerFoundationIcons()->GetSlicerInvisibleIcon()  );
  this->AllVisibilityMenuButton->GetMenu()->SetItemCompoundModeToLeft ( index );
  this->AllVisibilityMenuButton->GetMenu()->SetItemIndicatorVisibility ( index, 0);
  this->AllVisibilityMenuButton->GetMenu()->AddSeparator();
  this->AllVisibilityMenuButton->GetMenu()->AddRadioButton ( "close");
  index = this->AllVisibilityMenuButton->GetMenu()->GetIndexOfItem ("close");
  this->AllVisibilityMenuButton->GetMenu()->SetItemIndicatorVisibility ( index, 0);

  this->Script("pack %s -side left -anchor w -padx 2 -pady 2",
              this->AllVisibilityMenuButton->GetWidgetName() );
  
  // ---
  // CHOOSE Angle Node FRAME
  vtkSlicerModuleCollapsibleFrame *pickAngleNodeFrame = vtkSlicerModuleCollapsibleFrame::New ( );
  pickAngleNodeFrame->SetParent ( this->GetParent() );
  pickAngleNodeFrame->Create ( );
  pickAngleNodeFrame->SetLabelText("Modify Selected Angle Node");
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 pickAngleNodeFrame->GetWidgetName() );
  
   // a selector to pick a angle
  this->AngleSelectorWidget = vtkSlicerNodeSelectorWidget::New() ;
  this->AngleSelectorWidget->SetParent ( pickAngleNodeFrame->GetFrame() );
  this->AngleSelectorWidget->Create ( );
  this->AngleSelectorWidget->SetNodeClass("vtkMRMLMeasurementsAngleNode", NULL, NULL, NULL);
  this->AngleSelectorWidget->NewNodeEnabledOn();
  this->AngleSelectorWidget->SetMRMLScene(this->GetMRMLScene());
  this->AngleSelectorWidget->SetBorderWidth(2);
  // this->AngleSelectorWidget->SetReliefToGroove();
  this->AngleSelectorWidget->SetPadX(2);
  this->AngleSelectorWidget->SetPadY(2);
  this->AngleSelectorWidget->GetWidget()->GetWidget()->IndicatorVisibilityOff();
  this->AngleSelectorWidget->GetWidget()->GetWidget()->SetWidth(24);
  this->AngleSelectorWidget->SetLabelText( "Angle Node Select: ");
  this->AngleSelectorWidget->SetBalloonHelpString("select a angle node from the current mrml scene (currently only one 3D angle widget will be displayed at a time, it will update from the selected node).");
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 this->AngleSelectorWidget->GetWidgetName());

  
  this->VisibilityButton = vtkKWCheckButtonWithLabel::New();
  this->VisibilityButton->SetParent ( pickAngleNodeFrame->GetFrame() );
  this->VisibilityButton->Create ( );
  this->VisibilityButton->SetLabelText("Toggle Visibility");
  this->VisibilityButton->SetBalloonHelpString("set widget visibility.");
  this->Script ( "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2",
                 this->VisibilityButton->GetWidgetName() );
  
  // position 1 frame
  vtkKWFrame *position1Frame = vtkKWFrame::New();
  position1Frame->SetParent(pickAngleNodeFrame->GetFrame());
  position1Frame->Create();
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 position1Frame->GetWidgetName() );
  
  this->Position1Label = vtkKWLabel::New();
  this->Position1Label->SetParent(position1Frame);
  this->Position1Label->Create();
  this->Position1Label->SetText("Postion 1");

  this->Position1XEntry = vtkKWEntry::New();
  this->Position1XEntry->SetParent(position1Frame);
  this->Position1XEntry->Create();
  this->Position1XEntry->SetWidth(8);
  this->Position1XEntry->SetRestrictValueToDouble();
  this->Position1XEntry->SetBalloonHelpString("First end of the angle, X position");

  this->Position1YEntry = vtkKWEntry::New();
  this->Position1YEntry->SetParent(position1Frame);
  this->Position1YEntry->Create();
  this->Position1YEntry->SetWidth(8);
  this->Position1YEntry->SetRestrictValueToDouble();
  this->Position1YEntry->SetBalloonHelpString("First end of the angle, Y position");

  this->Position1ZEntry = vtkKWEntry::New();
  this->Position1ZEntry->SetParent(position1Frame);
  this->Position1ZEntry->Create();
  this->Position1ZEntry->SetWidth(8);
  this->Position1ZEntry->SetRestrictValueToDouble();
  this->Position1ZEntry->SetBalloonHelpString("First end of the angle, Z position");

  this->Script( "pack %s %s %s %s -side left -anchor nw -expand y -fill x -padx 2 -pady 2",
                  this->Position1Label->GetWidgetName(),
                  this->Position1XEntry->GetWidgetName(),
                  this->Position1YEntry->GetWidgetName(),
                  this->Position1ZEntry->GetWidgetName());

  // position 2 frame
  vtkKWFrame *position2Frame = vtkKWFrame::New();
  position2Frame->SetParent(pickAngleNodeFrame->GetFrame());
  position2Frame->Create();
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 position2Frame->GetWidgetName() );

  this->Position2Label = vtkKWLabel::New();
  this->Position2Label->SetParent(position2Frame);
  this->Position2Label->Create();
  this->Position2Label->SetText("Postion 2");

  this->Position2XEntry = vtkKWEntry::New();
  this->Position2XEntry->SetParent(position2Frame);
  this->Position2XEntry->Create();
  this->Position2XEntry->SetWidth(8);
  this->Position2XEntry->SetRestrictValueToDouble();
  this->Position2XEntry->SetBalloonHelpString("Second end of the angle, X position");

  this->Position2YEntry = vtkKWEntry::New();
  this->Position2YEntry->SetParent(position2Frame);
  this->Position2YEntry->Create();
  this->Position2YEntry->SetWidth(8);
  this->Position2YEntry->SetRestrictValueToDouble();
  this->Position2YEntry->SetBalloonHelpString("Second end of the angle, Y position");

  this->Position2ZEntry = vtkKWEntry::New();
  this->Position2ZEntry->SetParent(position2Frame);
  this->Position2ZEntry->Create();
  this->Position2ZEntry->SetWidth(8);
  this->Position2ZEntry->SetRestrictValueToDouble();
  this->Position2ZEntry->SetBalloonHelpString("Second end of the angle, Z position");
  this->Script( "pack %s %s %s %s -side left -anchor nw -expand y -fill x -padx 2 -pady 2",
                  this->Position2Label->GetWidgetName(),
                  this->Position2XEntry->GetWidgetName(),
                  this->Position2YEntry->GetWidgetName(),
                  this->Position2ZEntry->GetWidgetName());

  // position Center frame
  vtkKWFrame *positionCenterFrame = vtkKWFrame::New();
  positionCenterFrame->SetParent(pickAngleNodeFrame->GetFrame());
  positionCenterFrame->Create();
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 positionCenterFrame->GetWidgetName() );

  this->PositionCenterLabel = vtkKWLabel::New();
  this->PositionCenterLabel->SetParent(positionCenterFrame);
  this->PositionCenterLabel->Create();
  this->PositionCenterLabel->SetText("Center   ");

  this->PositionCenterXEntry = vtkKWEntry::New();
  this->PositionCenterXEntry->SetParent(positionCenterFrame);
  this->PositionCenterXEntry->Create();
  this->PositionCenterXEntry->SetWidth(8);
  this->PositionCenterXEntry->SetRestrictValueToDouble();
  this->PositionCenterXEntry->SetBalloonHelpString("Center of the angle, X position");

  this->PositionCenterYEntry = vtkKWEntry::New();
  this->PositionCenterYEntry->SetParent(positionCenterFrame);
  this->PositionCenterYEntry->Create();
  this->PositionCenterYEntry->SetWidth(8);
  this->PositionCenterYEntry->SetRestrictValueToDouble();
  this->PositionCenterYEntry->SetBalloonHelpString("Center of the angle, Y position");

  this->PositionCenterZEntry = vtkKWEntry::New();
  this->PositionCenterZEntry->SetParent(positionCenterFrame);
  this->PositionCenterZEntry->Create();
  this->PositionCenterZEntry->SetWidth(8);
  this->PositionCenterZEntry->SetRestrictValueToDouble();
  this->PositionCenterZEntry->SetBalloonHelpString("Center of the angle, Z position");
  this->Script( "pack %s %s %s %s -side left -anchor nw -expand y -fill x -padx 2 -pady 2",
                  this->PositionCenterLabel->GetWidgetName(),
                  this->PositionCenterXEntry->GetWidgetName(),
                  this->PositionCenterYEntry->GetWidgetName(),
                  this->PositionCenterZEntry->GetWidgetName());
  
  //
  // Pick Models Frame
  //
  vtkKWFrameWithLabel *modelFrame = vtkKWFrameWithLabel::New();
  modelFrame->SetParent( pickAngleNodeFrame->GetFrame() );
  modelFrame->Create();
  modelFrame->SetLabelText("Constrain Angle to Models");
  modelFrame->ExpandFrame();
  this->Script("pack %s -side top -anchor nw -fill x -padx 2 -pady 2", modelFrame->GetWidgetName());
  modelFrame->CollapseFrame();
  

  this->AngleModel1SelectorWidget = vtkSlicerNodeSelectorWidget::New() ;
  this->AngleModel1SelectorWidget->SetParent ( modelFrame->GetFrame() );
  this->AngleModel1SelectorWidget->Create ( );
  this->AngleModel1SelectorWidget->AddNodeClass("vtkMRMLModelNode", NULL, NULL, NULL);
  this->AngleModel1SelectorWidget->AddNodeClass("vtkMRMLSliceNode", NULL, NULL, NULL);
  this->AngleModel1SelectorWidget->SetChildClassesEnabled(1);
  this->AngleModel1SelectorWidget->NoneEnabledOn();
  this->AngleModel1SelectorWidget->SetShowHidden(1);
  this->AngleModel1SelectorWidget->SetMRMLScene(this->GetMRMLScene());
  this->AngleModel1SelectorWidget->SetBorderWidth(2);
  this->AngleModel1SelectorWidget->SetPadX(2);
  this->AngleModel1SelectorWidget->SetPadY(2);
  this->AngleModel1SelectorWidget->GetWidget()->GetWidget()->IndicatorVisibilityOff();
  this->AngleModel1SelectorWidget->GetWidget()->GetWidget()->SetWidth(24);
  this->AngleModel1SelectorWidget->SetLabelText( "Select Angle Model 1: ");
  this->AngleModel1SelectorWidget->SetBalloonHelpString("Select a model on which to anchor the first end of the angle.");
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 this->AngleModel1SelectorWidget->GetWidgetName());
  
  this->AngleModel2SelectorWidget = vtkSlicerNodeSelectorWidget::New() ;
  this->AngleModel2SelectorWidget->SetParent ( modelFrame->GetFrame() );
  this->AngleModel2SelectorWidget->Create ( );
  this->AngleModel2SelectorWidget->AddNodeClass("vtkMRMLModelNode", NULL, NULL, NULL);
  this->AngleModel2SelectorWidget->AddNodeClass("vtkMRMLSliceNode", NULL, NULL, NULL);
  this->AngleModel2SelectorWidget->SetChildClassesEnabled(1);
  this->AngleModel2SelectorWidget->NoneEnabledOn();
  this->AngleModel2SelectorWidget->SetShowHidden(1);
  this->AngleModel2SelectorWidget->SetMRMLScene(this->GetMRMLScene());
  this->AngleModel2SelectorWidget->SetBorderWidth(2);
  this->AngleModel2SelectorWidget->SetPadX(2);
  this->AngleModel2SelectorWidget->SetPadY(2);
  this->AngleModel2SelectorWidget->GetWidget()->GetWidget()->IndicatorVisibilityOff();
  this->AngleModel2SelectorWidget->GetWidget()->GetWidget()->SetWidth(24);
  this->AngleModel2SelectorWidget->SetLabelText( "Select Angle Model 2: ");
  this->AngleModel2SelectorWidget->SetBalloonHelpString("Select a model on which to anchor the second end of the angle.");
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 this->AngleModel2SelectorWidget->GetWidgetName());

  this->AngleModelCenterSelectorWidget = vtkSlicerNodeSelectorWidget::New() ;
  this->AngleModelCenterSelectorWidget->SetParent ( modelFrame->GetFrame() );
  this->AngleModelCenterSelectorWidget->Create ( );
  this->AngleModelCenterSelectorWidget->AddNodeClass("vtkMRMLModelNode", NULL, NULL, NULL);
  this->AngleModelCenterSelectorWidget->AddNodeClass("vtkMRMLSliceNode", NULL, NULL, NULL);
  this->AngleModelCenterSelectorWidget->SetChildClassesEnabled(1);
  this->AngleModelCenterSelectorWidget->NoneEnabledOn();
  this->AngleModelCenterSelectorWidget->SetShowHidden(1);
  this->AngleModelCenterSelectorWidget->SetMRMLScene(this->GetMRMLScene());
  this->AngleModelCenterSelectorWidget->SetBorderWidth(2);
  this->AngleModelCenterSelectorWidget->SetPadX(2);
  this->AngleModelCenterSelectorWidget->SetPadY(2);
  this->AngleModelCenterSelectorWidget->GetWidget()->GetWidget()->IndicatorVisibilityOff();
  this->AngleModelCenterSelectorWidget->GetWidget()->GetWidget()->SetWidth(24);
  this->AngleModelCenterSelectorWidget->SetLabelText( "Select Angle Model Center: ");
  this->AngleModelCenterSelectorWidget->SetBalloonHelpString("Select a model on which to anchor the center  of the angle.");
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 this->AngleModelCenterSelectorWidget->GetWidgetName());

  // ---
  // DISPLAY FRAME            
  vtkKWFrameWithLabel *angleDisplayFrame = vtkKWFrameWithLabel::New ( );
  angleDisplayFrame->SetParent ( pickAngleNodeFrame->GetFrame() );
  angleDisplayFrame->SetLabelText("Display Options");
  angleDisplayFrame->Create ( );
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 angleDisplayFrame->GetWidgetName() );
  angleDisplayFrame->CollapseFrame ( );

  this->PointColourButton = vtkKWChangeColorButton::New();
  this->PointColourButton->SetParent ( angleDisplayFrame->GetFrame() );
  this->PointColourButton->Create ( );
  this->PointColourButton->SetColor(0.0, 0.0, 1.0);
  this->PointColourButton->LabelOutsideButtonOn();
  this->PointColourButton->SetLabelPositionToRight();
  this->PointColourButton->SetLabelText("Set End Point Color");
  this->PointColourButton->SetBalloonHelpString("set point color.");
  this->Script ( "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2",
                 this->PointColourButton->GetWidgetName() );

  this->LineColourButton = vtkKWChangeColorButton::New();
  this->LineColourButton->SetParent ( angleDisplayFrame->GetFrame() );
  this->LineColourButton->Create ( );
  this->LineColourButton->SetColor(1.0, 1.0, 1.0);
  this->LineColourButton->LabelOutsideButtonOn();
  this->LineColourButton->SetLabelPositionToRight();
  this->LineColourButton->SetLabelText("Set Line Color");
  this->LineColourButton->SetBalloonHelpString("set line color.");
  this->Script ( "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2",
                 this->LineColourButton->GetWidgetName() );
  
  // angle annotation frame
  vtkKWFrame *annotationFrame = vtkKWFrame::New();
  annotationFrame->SetParent(angleDisplayFrame->GetFrame());
  annotationFrame->Create();
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 annotationFrame->GetWidgetName() );

  // sub component visibility
  vtkKWFrame *visibilityFrame = vtkKWFrame::New();
  visibilityFrame->SetParent(annotationFrame);
  visibilityFrame->Create();
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 visibilityFrame->GetWidgetName() );

  this->LabelVisibilityButton = vtkKWCheckButtonWithLabel::New();
  this->LabelVisibilityButton->SetParent ( visibilityFrame );
  this->LabelVisibilityButton->Create ( );
  this->LabelVisibilityButton->SetLabelText("Toggle Angle Annotation Visibility");
  this->LabelVisibilityButton->SetBalloonHelpString("set angle annotation visibility.");
//  this->Script ( "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2",
//                 this->LabelVisibilityButton->GetWidgetName() );
  
  this->Ray1VisibilityButton = vtkKWCheckButtonWithLabel::New();
  this->Ray1VisibilityButton->SetParent ( visibilityFrame );
  this->Ray1VisibilityButton->Create ( );
  this->Ray1VisibilityButton->SetLabelText("Toggle Ray 1 Visibility");
  this->Ray1VisibilityButton->SetBalloonHelpString("set ray 1 visibility.");
  this->Script ( "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2",
                 this->Ray1VisibilityButton->GetWidgetName() );

  this->Ray2VisibilityButton = vtkKWCheckButtonWithLabel::New();
  this->Ray2VisibilityButton->SetParent ( visibilityFrame );
  this->Ray2VisibilityButton->Create ( );
  this->Ray2VisibilityButton->SetLabelText("Toggle Ray 2 Visibility");
  this->Ray2VisibilityButton->SetBalloonHelpString("set ray 2 visibility.");
  this->Script ( "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2",
                 this->Ray2VisibilityButton->GetWidgetName() );

  this->ArcVisibilityButton = vtkKWCheckButtonWithLabel::New();
  this->ArcVisibilityButton->SetParent ( visibilityFrame );
  this->ArcVisibilityButton->Create ( );
  this->ArcVisibilityButton->SetLabelText("Toggle Arc Visibility");
  this->ArcVisibilityButton->SetBalloonHelpString("set arc visibility.");
  this->Script ( "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2",
                 this->ArcVisibilityButton->GetWidgetName() );
  

  this->TextColourButton = vtkKWChangeColorButton::New();
  this->TextColourButton->SetParent ( annotationFrame );
  this->TextColourButton->Create ( );
  this->TextColourButton->SetColor(1.0, 1.0, 1.0);
  this->TextColourButton->LabelOutsideButtonOn();
  this->TextColourButton->SetLabelPositionToRight();
  this->TextColourButton->SetLabelText("Set Text Color");
  this->TextColourButton->SetBalloonHelpString("set text color.");
  // commented out until VTK supports getting the angle annotation text propery
  //this->Script ( "pack %s -side top -anchor nw -expand y -fill x -padx 2 -pady 2",
  //               this->TextColourButton->GetWidgetName() );
  
  this->LabelFormatEntry = vtkKWEntryWithLabel::New();
  this->LabelFormatEntry->SetParent(annotationFrame);
  this->LabelFormatEntry->Create();
  this->LabelFormatEntry->SetLabelText("Angle Annotation Format");
  this->LabelFormatEntry->SetBalloonHelpString("string formatting command, use %g to print out angle in a default floating point format, %.1f to print out only one digit after the decimal, plus any text you wish");
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 this->LabelFormatEntry->GetWidgetName());

  this->LabelScaleEntry =  vtkKWEntryWithLabel::New();
  this->LabelScaleEntry->SetParent(annotationFrame);
  this->LabelScaleEntry->Create();
  this->LabelScaleEntry->SetLabelText("Angle Annotation Scale");
  this->LabelScaleEntry->GetWidget()->SetRestrictValueToDouble();
  this->LabelScaleEntry->SetBalloonHelpString("Scale value applied to the angle annotation text");
  this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
                 this->LabelScaleEntry->GetWidgetName());

  this->ResolutionEntry = vtkKWEntryWithLabel::New();
  this->ResolutionEntry->SetParent(annotationFrame);
  this->ResolutionEntry->Create();
  this->ResolutionEntry->SetLabelText("Resolution");
  this->ResolutionEntry->SetBalloonHelpString(" number of subdivisions on the line");
  this->ResolutionEntry->GetWidget()->SetRestrictValueToInteger();
  // this is not used with the current line widget
  //this->Script ( "pack %s -side top -anchor nw -fill x -padx 2 -pady 2",
  //               this->ResolutionEntry->GetWidgetName());
  
  // add observers
  this->AddWidgetObservers();

  modelFrame->Delete();
  angleDisplayFrame->Delete();
  visibilityFrame->Delete();
  position1Frame->Delete();
  position2Frame->Delete();
  positionCenterFrame->Delete();
  pickAngleNodeFrame->Delete();
  controlAllFrame->Delete();
  annotationFrame->Delete();
  
  // register node classes
  if (this->GetMRMLScene())
    {
    vtkMRMLMeasurementsAngleNode *angleNode = vtkMRMLMeasurementsAngleNode::New();
    this->GetMRMLScene()->RegisterNodeClass(angleNode);
    angleNode->Delete();
    angleNode = NULL;
    }
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::SetViewerWidget ( vtkSlicerViewerWidget *viewerWidget )
{
  this->ViewerWidget = viewerWidget;
}

//---------------------------------------------------------------------------
vtkMeasurementsAngleWidgetClass *vtkMeasurementsAngleWidget::GetAngleWidget(const char *nodeID)
{
  std::map<std::string, vtkMeasurementsAngleWidgetClass *>::iterator iter;
  for (iter = this->AngleWidgets.begin();
       iter != this->AngleWidgets.end();
       iter++)
    {
    if (iter->first.c_str() && !strcmp(iter->first.c_str(), nodeID))
      {
      return iter->second;
      }
    }
  return NULL;
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::AddAngleWidget(vtkMRMLMeasurementsAngleNode *angleNode)
{
  if (!angleNode)
    {
    return;
    }
  if (this->GetAngleWidget(angleNode->GetID()) != NULL)
    {
    vtkDebugMacro("Already have widgets for angle node " << angleNode->GetID());
    return;
    }

  vtkDebugMacro("Creating new angle widget class");
  vtkMeasurementsAngleWidgetClass *c = vtkMeasurementsAngleWidgetClass::New();
  this->AngleWidgets[angleNode->GetID()] = c;
  // make sure we're observing the node for transform changes
  if (angleNode->HasObserver(vtkMRMLTransformableNode::TransformModifiedEvent, (vtkCommand *)this->MRMLCallbackCommand) != 1)
    {
    angleNode->AddObserver(vtkMRMLTransformableNode::TransformModifiedEvent, (vtkCommand *)this->MRMLCallbackCommand);
    }
  if (angleNode->HasObserver(vtkCommand::ModifiedEvent, (vtkCommand *)this->MRMLCallbackCommand) != 1)
    {
    angleNode->AddObserver(vtkCommand::ModifiedEvent, (vtkCommand *)this->MRMLCallbackCommand);
    }
  //vtkIntArray *events = vtkIntArray::New();
  //events->InsertNextValue(vtkCommand::ModifiedEvent);
//  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
//  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  //events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  //vtkSetAndObserveMRMLNodeEventsMacro(NULL, angleNode, events);
  //events->Delete();
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::RemoveAngleWidget(vtkMRMLMeasurementsAngleNode *angleNode)
{
  if (!angleNode)
    {
    return;
    }
  if (this->GetAngleWidget(angleNode->GetID()) != NULL)
    {
    this->AngleWidgets[angleNode->GetID()]->Delete();
    this->AngleWidgets.erase(angleNode->GetID());
    }
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::Update3DWidgetsFromMRML()
{
  if (!this->MRMLScene)
    {
      vtkDebugMacro("UpdateFromMRML: no mrml scene from which to update!");
      return;
    }

  int nnodes = this->MRMLScene->GetNumberOfNodesByClass("vtkMRMLMeasurementsAngleNode");
  vtkDebugMacro("UpdateFromMRML: have " << nnodes << " angle nodes in the scene, " << this->AngleWidgets.size() << " widgets defined already");

  for (int n=0; n<nnodes; n++)
    {
    vtkMRMLMeasurementsAngleNode *rnode = vtkMRMLMeasurementsAngleNode::SafeDownCast(this->MRMLScene->GetNthNodeByClass(n, "vtkMRMLMeasurementsAngleNode"));
    if (rnode)
      {
      // this call will create one if it's missing
      this->Update3DWidget(rnode);
      // let go of the pointer
      rnode = NULL;
      }
    }

  // now have a widget for each node, check that don't have too many widgets
  if ((int)(this->AngleWidgets.size()) != nnodes)
    {
    vtkDebugMacro("UpdateFromMRML: after adding widgets for scene nodes, have " << this->AngleWidgets.size() << " instead of " << nnodes);
    // find ones that aren't in the scene, be careful using an iterator because calling erase gets it messed up
    //int numWidgets = this->AngleWidgets.size();
    std::map<std::string, vtkMeasurementsAngleWidgetClass *>::iterator iter;
    std::vector<std::string> idsToDelete;
    for (iter = this->AngleWidgets.begin();
         iter != this->AngleWidgets.end();
         iter++)
      {
      if (this->MRMLScene->GetNodeByID(iter->first.c_str()) == NULL)
        {
        vtkDebugMacro("UpdateFromMRML: found an extra widget with id " << iter->first.c_str());
        // add it to a list and do delete and erase in a second round
        idsToDelete.push_back(iter->first);
        }
      }
    for (int i = 0; i < (int)(idsToDelete.size()); i++)
      {
      std::map<std::string, vtkMeasurementsAngleWidgetClass *>::iterator delIter;
      delIter = this->AngleWidgets.find(idsToDelete[i]);
      if (delIter != this->AngleWidgets.end())
        {
        // can't call this->RemoveAngleWidget because we don't have a node!
        this->AngleWidgets[delIter->first.c_str()]->Delete();
        this->AngleWidgets.erase(delIter->first.c_str());
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkMeasurementsAngleWidget::ModifyAllAngleVisibility( int visibilityState)
{
  if ( this->MRMLScene == NULL )
    {
    vtkErrorMacro ( "ModifyAllAngleVisibility: got NULL MRMLScene." );
    return;
    }
  if ( visibilityState != 0 && visibilityState != 1 )
    {
    vtkErrorMacro ( "ModifyAllAngleVisibility: got bad value for lock state; should be 0 or 1" );
    return;
    }
  
  vtkMRMLMeasurementsAngleNode *angleNode;
  
  // save state for undo:
  // maybe we should just make a list of all the angle nodes
  // and save their state here instead of the entire scene?
  this->MRMLScene->SaveStateForUndo();
  int numnodes = this->MRMLScene->GetNumberOfNodesByClass ( "vtkMRMLMeasurementsAngleNode" );
  for ( int nn=0; nn<numnodes; nn++ )
    {
    angleNode = vtkMRMLMeasurementsAngleNode::SafeDownCast (this->MRMLScene->GetNthNodeByClass ( nn, "vtkMRMLMeasurementsAngleNode" ));
    if ( angleNode != NULL )
      {
      angleNode->SetVisibility ( visibilityState );
      }
    }
}
