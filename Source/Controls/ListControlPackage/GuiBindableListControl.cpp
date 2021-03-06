#include "GuiBindableListControl.h"
#include "../Templates/GuiControlTemplateStyles.h"

namespace vl
{
	namespace presentation
	{
		namespace controls
		{
			using namespace collections;
			using namespace list;
			using namespace tree;
			using namespace reflection::description;
			using namespace templates;

			Value ReadProperty(const Value& thisObject, const WString& propertyName)
			{
				if (!thisObject.IsNull() && propertyName != L"")
				{
					auto td = thisObject.GetTypeDescriptor();
					auto info = td->GetPropertyByName(propertyName, true);
					if (info && info->IsReadable())
					{
						return info->GetValue(thisObject);
					}
					else
					{
						return Value();
					}
				}
				return thisObject;
			}

			void WriteProperty(Value& thisObject, const WString& propertyName, const Value& newValue)
			{
				if (!thisObject.IsNull() && propertyName != L"")
				{
					auto td = thisObject.GetTypeDescriptor();
					auto info = td->GetPropertyByName(propertyName, true);
					if (info && info->IsWritable())
					{
						info->SetValue(thisObject, newValue);
					}
				}
			}

/***********************************************************************
GuiBindableTextList::ItemSource
***********************************************************************/

			GuiBindableTextList::ItemSource::ItemSource()
			{
			}

			GuiBindableTextList::ItemSource::~ItemSource()
			{
				SetItemSource(nullptr);
			}

			Ptr<description::IValueEnumerable> GuiBindableTextList::ItemSource::GetItemSource()
			{
				return itemSource;
			}

			void GuiBindableTextList::ItemSource::SetItemSource(Ptr<description::IValueEnumerable> _itemSource)
			{
				vint oldCount = 0;
				if (itemSource)
				{
					oldCount = itemSource->GetCount();
				}
				if (itemChangedEventHandler)
				{
					auto ol = itemSource.Cast<IValueObservableList>();
					ol->ItemChanged.Remove(itemChangedEventHandler);
				}

				itemSource = nullptr;
				itemChangedEventHandler = nullptr;

				if (_itemSource)
				{
					if (auto ol = _itemSource.Cast<IValueObservableList>())
					{
						itemSource = ol;
						itemChangedEventHandler = ol->ItemChanged.Add([this](vint start, vint oldCount, vint newCount)
						{
							InvokeOnItemModified(start, oldCount, newCount);
						});
					}
					else if (auto rl = _itemSource.Cast<IValueReadonlyList>())
					{
						itemSource = rl;
					}
					else
					{
						itemSource = IValueList::Create(GetLazyList<Value>(_itemSource));
					}
				}

				InvokeOnItemModified(0, oldCount, itemSource ? itemSource->GetCount() : 0);
			}

			description::Value GuiBindableTextList::ItemSource::Get(vint index)
			{
				if (!itemSource) return Value();
				return itemSource->Get(index);
			}

			void GuiBindableTextList::ItemSource::UpdateBindingProperties()
			{
				InvokeOnItemModified(0, Count(), Count());
			}
					
			// ===================== GuiListControl::IItemProvider =====================
			
			vint GuiBindableTextList::ItemSource::Count()
			{
				if (!itemSource) return 0;
				return itemSource->GetCount();
			}
			
			IDescriptable* GuiBindableTextList::ItemSource::RequestView(const WString& identifier)
			{
				if (identifier == GuiListControl::IItemBindingView::Identifier)
				{
					return (GuiListControl::IItemBindingView*)this;
				}
				else if (identifier == GuiListControl::IItemPrimaryTextView::Identifier)
				{
					return (GuiListControl::IItemPrimaryTextView*)this;
				}
				else if (identifier == TextItemStyleProvider::ITextItemView::Identifier)
				{
					return (TextItemStyleProvider::ITextItemView*)this;
				}
				else
				{
					return 0;
				}
			}
			
			void GuiBindableTextList::ItemSource::ReleaseView(IDescriptable* view)
			{
			}
					
			// ===================== GuiListControl::IItemBindingView =====================

			description::Value GuiBindableTextList::ItemSource::GetBindingValue(vint itemIndex)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount())
					{
						return itemSource->Get(itemIndex);
					}
				}
				return Value();
			}
					
			// ===================== GuiListControl::IItemPrimaryTextView =====================

			WString GuiBindableTextList::ItemSource::GetPrimaryTextViewText(vint itemIndex)
			{
				return GetText(itemIndex);
			}
			
			bool GuiBindableTextList::ItemSource::ContainsPrimaryText(vint itemIndex)
			{
				if (!itemSource) return false;
				return 0 <= itemIndex && itemIndex < itemSource->GetCount();
			}
					
			// ===================== list::TextItemStyleProvider::ITextItemView =====================

			WString GuiBindableTextList::ItemSource::GetText(vint itemIndex)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount())
					{
						return ReadProperty(itemSource->Get(itemIndex), textProperty).GetText();
					}
				}
				return L"";
			}
			
			bool GuiBindableTextList::ItemSource::GetChecked(vint itemIndex)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount())
					{
						auto value = ReadProperty(itemSource->Get(itemIndex), checkedProperty);
						if (value.GetTypeDescriptor() == description::GetTypeDescriptor<bool>())
						{
							return UnboxValue<bool>(value);
						}
					}
				}
				return false;
			}
			
			void GuiBindableTextList::ItemSource::SetCheckedSilently(vint itemIndex, bool value)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount())
					{
						auto thisValue = itemSource->Get(itemIndex);
						WriteProperty(thisValue, checkedProperty, BoxValue(value));
					}
				}
			}

/***********************************************************************
GuiBindableTextList
***********************************************************************/

			GuiBindableTextList::GuiBindableTextList(IStyleProvider* _styleProvider, list::TextItemStyleProvider::IBulletFactory* _bulletFactory)
				:GuiVirtualTextList(_styleProvider, _bulletFactory, new ItemSource)
			{
				itemSource = dynamic_cast<ItemSource*>(GetItemProvider());

				TextPropertyChanged.SetAssociatedComposition(boundsComposition);
				TextPropertyChanged.SetAssociatedComposition(boundsComposition);
			}

			GuiBindableTextList::~GuiBindableTextList()
			{
			}

			Ptr<description::IValueEnumerable> GuiBindableTextList::GetItemSource()
			{
				return itemSource->GetItemSource();
			}

			void GuiBindableTextList::SetItemSource(Ptr<description::IValueEnumerable> _itemSource)
			{
				itemSource->SetItemSource(_itemSource);
			}

			const WString& GuiBindableTextList::GetTextProperty()
			{
				return itemSource->textProperty;
			}

			void GuiBindableTextList::SetTextProperty(const WString& value)
			{
				if (itemSource->textProperty != value)
				{
					itemSource->textProperty = value;
					itemSource->UpdateBindingProperties();
					TextPropertyChanged.Execute(GetNotifyEventArguments());
				}
			}

			const WString& GuiBindableTextList::GetCheckedProperty()
			{
				return itemSource->checkedProperty;
			}

			void GuiBindableTextList::SetCheckedProperty(const WString& value)
			{
				if (itemSource->checkedProperty != value)
				{
					itemSource->checkedProperty = value;
					itemSource->UpdateBindingProperties();
					CheckedPropertyChanged.Execute(GetNotifyEventArguments());
				}
			}

			description::Value GuiBindableTextList::GetSelectedItem()
			{
				vint index = GetSelectedItemIndex();
				if (index == -1) return Value();
				return itemSource->Get(index);
			}

/***********************************************************************
GuiBindableListView::ItemSource
***********************************************************************/

			GuiBindableListView::ItemSource::ItemSource()
				:columns(this)
				, dataColumns(this)
			{
			}

			GuiBindableListView::ItemSource::~ItemSource()
			{
				SetItemSource(nullptr);
			}

			Ptr<description::IValueEnumerable> GuiBindableListView::ItemSource::GetItemSource()
			{
				return itemSource;
			}

			void GuiBindableListView::ItemSource::SetItemSource(Ptr<description::IValueEnumerable> _itemSource)
			{
				vint oldCount = 0;
				if (itemSource)
				{
					oldCount = itemSource->GetCount();
				}
				if (itemChangedEventHandler)
				{
					auto ol = itemSource.Cast<IValueObservableList>();
					ol->ItemChanged.Remove(itemChangedEventHandler);
				}

				itemSource = nullptr;
				itemChangedEventHandler = nullptr;

				if (_itemSource)
				{
					if (auto ol = _itemSource.Cast<IValueObservableList>())
					{
						itemSource = ol;
						itemChangedEventHandler = ol->ItemChanged.Add([this](vint start, vint oldCount, vint newCount)
						{
							InvokeOnItemModified(start, oldCount, newCount);
						});
					}
					else if (auto rl = _itemSource.Cast<IValueReadonlyList>())
					{
						itemSource = rl;
					}
					else
					{
						itemSource = IValueList::Create(GetLazyList<Value>(_itemSource));
					}
				}

				InvokeOnItemModified(0, oldCount, itemSource ? itemSource->GetCount() : 0);
			}

			description::Value GuiBindableListView::ItemSource::Get(vint index)
			{
				if (!itemSource) return Value();
				return itemSource->Get(index);
			}

			void GuiBindableListView::ItemSource::UpdateBindingProperties()
			{
				InvokeOnItemModified(0, Count(), Count());
			}

			bool GuiBindableListView::ItemSource::NotifyUpdate(vint start, vint count)
			{
				if (!itemSource) return false;
				if (start<0 || start >= itemSource->GetCount() || count <= 0 || start + count > itemSource->GetCount())
				{
					return false;
				}
				else
				{
					InvokeOnItemModified(start, count, count);
					return true;
				}
			}

			list::ListViewDataColumns& GuiBindableListView::ItemSource::GetDataColumns()
			{
				return dataColumns;
			}

			list::ListViewColumns& GuiBindableListView::ItemSource::GetColumns()
			{
				return columns;
			}
					
			// ===================== list::IListViewItemProvider =====================

			void GuiBindableListView::ItemSource::NotifyAllItemsUpdate()
			{
				NotifyUpdate(0, Count());
			}

			void GuiBindableListView::ItemSource::NotifyAllColumnsUpdate()
			{
				for (vint i = 0; i < columnItemViewCallbacks.Count(); i++)
				{
					columnItemViewCallbacks[i]->OnColumnChanged();
				}
			}

			// ===================== GuiListControl::IItemProvider =====================

			vint GuiBindableListView::ItemSource::Count()
			{
				if (!itemSource) return 0;
				return itemSource->GetCount();
			}

			IDescriptable* GuiBindableListView::ItemSource::RequestView(const WString& identifier)
			{
				if (identifier == GuiListControl::IItemBindingView::Identifier)
				{
					return (GuiListControl::IItemBindingView*)this;
				}
				else if(identifier==ListViewItemStyleProvider::IListViewItemView::Identifier)
				{
					return (ListViewItemStyleProvider::IListViewItemView*)this;
				}
				else if(identifier==ListViewColumnItemArranger::IColumnItemView::Identifier)
				{
					return (ListViewColumnItemArranger::IColumnItemView*)this;
				}
				else if(identifier==GuiListControl::IItemPrimaryTextView::Identifier)
				{
					return (GuiListControl::IItemPrimaryTextView*)this;
				}
				else
				{
					return 0;
				}
			}

			void GuiBindableListView::ItemSource::ReleaseView(IDescriptable* view)
			{
			}
					
			// ===================== GuiListControl::IItemBindingView =====================

			description::Value GuiBindableListView::ItemSource::GetBindingValue(vint itemIndex)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount())
					{
						return itemSource->Get(itemIndex);
					}
				}
				return Value();
			}

			// ===================== GuiListControl::IItemPrimaryTextView =====================

			WString GuiBindableListView::ItemSource::GetPrimaryTextViewText(vint itemIndex)
			{
				return GetText(itemIndex);
			}

			bool GuiBindableListView::ItemSource::ContainsPrimaryText(vint itemIndex)
			{
				return true;
			}

			// ===================== list::ListViewItemStyleProvider::IListViewItemView =====================

			Ptr<GuiImageData> GuiBindableListView::ItemSource::GetSmallImage(vint itemIndex)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount())
					{
						auto value = ReadProperty(itemSource->Get(itemIndex), smallImageProperty);
						return value.GetSharedPtr().Cast<GuiImageData>();
					}
				}
				return nullptr;
			}

			Ptr<GuiImageData> GuiBindableListView::ItemSource::GetLargeImage(vint itemIndex)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount())
					{
						auto value = ReadProperty(itemSource->Get(itemIndex), largeImageProperty);
						return value.GetSharedPtr().Cast<GuiImageData>();
					}
				}
				return nullptr;
			}

			WString GuiBindableListView::ItemSource::GetText(vint itemIndex)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount() && columns.Count()>0)
					{
						return ReadProperty(itemSource->Get(itemIndex), columns[0]->GetTextProperty()).GetText();
					}
				}
				return L"";
			}

			WString GuiBindableListView::ItemSource::GetSubItem(vint itemIndex, vint index)
			{
				if (itemSource)
				{
					if (0 <= itemIndex && itemIndex < itemSource->GetCount() && 0 <= index && index < columns.Count() - 1)
					{
						return ReadProperty(itemSource->Get(itemIndex), columns[index + 1]->GetTextProperty()).GetText();
					}
				}
				return L"";
			}

			vint GuiBindableListView::ItemSource::GetDataColumnCount()
			{
				return dataColumns.Count();
			}

			vint GuiBindableListView::ItemSource::GetDataColumn(vint index)
			{
				return dataColumns[index];
			}

			// ===================== list::ListViewColumnItemArranger::IColumnItemView =====================

			bool GuiBindableListView::ItemSource::AttachCallback(ListViewColumnItemArranger::IColumnItemViewCallback* value)
			{
				if(columnItemViewCallbacks.Contains(value))
				{
					return false;
				}
				else
				{
					columnItemViewCallbacks.Add(value);
					return true;
				}
			}

			bool GuiBindableListView::ItemSource::DetachCallback(ListViewColumnItemArranger::IColumnItemViewCallback* value)
			{
				vint index = columnItemViewCallbacks.IndexOf(value);
				if (index == -1)
				{
					return false;
				}
				else
				{
					columnItemViewCallbacks.Remove(value);
					return true;
				}
			}

			vint GuiBindableListView::ItemSource::GetColumnCount()
			{
				return columns.Count();
			}

			WString GuiBindableListView::ItemSource::GetColumnText(vint index)
			{
				if (index < 0 || index >= columns.Count())
				{
					return L"";
				}
				else
				{
					return columns[index]->GetText();
				}
			}

			vint GuiBindableListView::ItemSource::GetColumnSize(vint index)
			{
				if (index < 0 || index >= columns.Count())
				{
					return 0;
				}
				else
				{
					return columns[index]->GetSize();
				}
			}

			void GuiBindableListView::ItemSource::SetColumnSize(vint index, vint value)
			{
				if (index >= 0 && index < columns.Count())
				{
					columns[index]->SetSize(value);
				}
			}

			GuiMenu* GuiBindableListView::ItemSource::GetDropdownPopup(vint index)
			{
				if (index < 0 || index >= columns.Count())
				{
					return 0;
				}
				else
				{
					return columns[index]->GetDropdownPopup();
				}
			}

			GuiListViewColumnHeader::ColumnSortingState GuiBindableListView::ItemSource::GetSortingState(vint index)
			{
				if (index < 0 || index >= columns.Count())
				{
					return GuiListViewColumnHeader::NotSorted;
				}
				else
				{
					return columns[index]->GetSortingState();
				}
			}

/***********************************************************************
GuiBindableListView
***********************************************************************/

			GuiBindableListView::GuiBindableListView(IStyleProvider* _styleProvider)
				:GuiVirtualListView(_styleProvider, new ItemSource)
			{
				itemSource = dynamic_cast<ItemSource*>(GetItemProvider());

				LargeImagePropertyChanged.SetAssociatedComposition(boundsComposition);
				SmallImagePropertyChanged.SetAssociatedComposition(boundsComposition);
			}

			GuiBindableListView::~GuiBindableListView()
			{
			}

			list::ListViewDataColumns& GuiBindableListView::GetDataColumns()
			{
				return itemSource->GetDataColumns();
			}

			list::ListViewColumns& GuiBindableListView::GetColumns()
			{
				return itemSource->GetColumns();
			}

			Ptr<description::IValueEnumerable> GuiBindableListView::GetItemSource()
			{
				return itemSource->GetItemSource();
			}

			void GuiBindableListView::SetItemSource(Ptr<description::IValueEnumerable> _itemSource)
			{
				itemSource->SetItemSource(_itemSource);
			}

			const WString& GuiBindableListView::GetLargeImageProperty()
			{
				return itemSource->largeImageProperty;
			}

			void GuiBindableListView::SetLargeImageProperty(const WString& value)
			{
				if (itemSource->largeImageProperty != value)
				{
					itemSource->largeImageProperty = value;
					itemSource->UpdateBindingProperties();
					LargeImagePropertyChanged.Execute(GetNotifyEventArguments());
				}
			}

			const WString& GuiBindableListView::GetSmallImageProperty()
			{
				return itemSource->smallImageProperty;
			}

			void GuiBindableListView::SetSmallImageProperty(const WString& value)
			{
				if (itemSource->smallImageProperty != value)
				{
					itemSource->smallImageProperty = value;
					itemSource->UpdateBindingProperties();
					SmallImagePropertyChanged.Execute(GetNotifyEventArguments());
				}
			}

			description::Value GuiBindableListView::GetSelectedItem()
			{
				vint index = GetSelectedItemIndex();
				if (index == -1) return Value();
				return itemSource->Get(index);
			}

/***********************************************************************
GuiBindableTreeView::ItemSourceNode
***********************************************************************/

			void GuiBindableTreeView::ItemSourceNode::PrepareChildren()
			{
				if (!childrenVirtualList)
				{
					auto value = ReadProperty(itemSource, rootProvider->childrenProperty);
					if (auto td = value.GetTypeDescriptor())
					{
						if (td->CanConvertTo(description::GetTypeDescriptor<IValueObservableList>()))
						{
							auto ol = UnboxValue<Ptr<IValueObservableList>>(value);
							itemChangedEventHandler = ol->ItemChanged.Add([this](vint start, vint oldCount, vint newCount)
							{
								callback->OnBeforeItemModified(this, start, oldCount, newCount);
								children.RemoveRange(start, oldCount);
								for (vint i = 0; i < newCount; i++)
								{
									Value value = childrenVirtualList->Get(start + i);
									auto node = new ItemSourceNode(value, this);
									children.Insert(start + i, node);
								}
								callback->OnAfterItemModified(this, start, oldCount, newCount);
							});
							childrenVirtualList = ol;
						}
						else if (td->CanConvertTo(description::GetTypeDescriptor<IValueReadonlyList>()))
						{
							childrenVirtualList = UnboxValue<Ptr<IValueReadonlyList>>(value);
						}
						else if (td->CanConvertTo(description::GetTypeDescriptor<IValueEnumerable>()))
						{
							auto e = UnboxValue<Ptr<IValueEnumerable>>(value);
							childrenVirtualList = IValueList::Create(GetLazyList<Value>(e));
						}
					}

					if (!childrenVirtualList)
					{
						childrenVirtualList = IValueList::Create();
					}

					vint count = childrenVirtualList->GetCount();
					for (vint i = 0; i < count; i++)
					{
						Value value = childrenVirtualList->Get(i);
						auto node = new ItemSourceNode(value, this);
						children.Add(node);
					}
				}
			}

			void GuiBindableTreeView::ItemSourceNode::UnprepareChildren()
			{
				if (itemChangedEventHandler)
				{
					auto ol = childrenVirtualList.Cast<IValueObservableList>();
					ol->ItemChanged.Remove(itemChangedEventHandler);
					itemChangedEventHandler = nullptr;
				}
				childrenVirtualList = nullptr;
				FOREACH(Ptr<ItemSourceNode>, node, children)
				{
					node->UnprepareChildren();
				}
				children.Clear();
			}

			GuiBindableTreeView::ItemSourceNode::ItemSourceNode(const description::Value& _itemSource, ItemSourceNode* _parent)
				:itemSource(_itemSource)
				, rootProvider(_parent->rootProvider)
				, parent(_parent)
				, callback(_parent->callback)
			{
			}

			GuiBindableTreeView::ItemSourceNode::ItemSourceNode(ItemSource* _rootProvider)
				:rootProvider(_rootProvider)
				, parent(nullptr)
				, callback(_rootProvider)
			{
			}

			GuiBindableTreeView::ItemSourceNode::~ItemSourceNode()
			{
				SetItemSource(Value());
			}

			description::Value GuiBindableTreeView::ItemSourceNode::GetItemSource()
			{
				return itemSource;
			}

			void GuiBindableTreeView::ItemSourceNode::SetItemSource(const description::Value& _itemSource)
			{
				vint oldCount = GetChildCount();
				UnprepareChildren();
				itemSource = _itemSource;
				vint newCount = GetChildCount();
				callback->OnBeforeItemModified(this, 0, oldCount, newCount);
				callback->OnAfterItemModified(this, 0, oldCount, newCount);
			}

			bool GuiBindableTreeView::ItemSourceNode::GetExpanding()
			{
				return this == rootProvider->rootNode.Obj() ? true : expanding;
			}

			void GuiBindableTreeView::ItemSourceNode::SetExpanding(bool value)
			{
				if (this != rootProvider->rootNode.Obj() && expanding != value)
				{
					expanding = value;
					if (expanding)
					{
						callback->OnItemExpanded(this);
					}
					else
					{
						callback->OnItemCollapsed(this);
					}
				}
			}

			vint GuiBindableTreeView::ItemSourceNode::CalculateTotalVisibleNodes()
			{
				if (!GetExpanding())
				{
					return 1;
				}

				PrepareChildren();
				vint count = 1;
				FOREACH(Ptr<ItemSourceNode>, child, children)
				{
					count += child->CalculateTotalVisibleNodes();
				}
				return count;
			}

			vint GuiBindableTreeView::ItemSourceNode::GetChildCount()
			{
				PrepareChildren();
				return children.Count();
			}

			tree::INodeProvider* GuiBindableTreeView::ItemSourceNode::GetParent()
			{
				return parent;
			}

			tree::INodeProvider* GuiBindableTreeView::ItemSourceNode::GetChild(vint index)
			{
				PrepareChildren();
				if (0 <= index && index < children.Count())
				{
					return children[index].Obj();
				}
				return 0;
			}

			void GuiBindableTreeView::ItemSourceNode::Increase()
			{
			}

			void GuiBindableTreeView::ItemSourceNode::Release()
			{
			}

/***********************************************************************
GuiBindableTreeView::ItemSource
***********************************************************************/

			GuiBindableTreeView::ItemSource::ItemSource()
			{
				rootNode = new ItemSourceNode(this);
			}

			GuiBindableTreeView::ItemSource::~ItemSource()
			{
			}

			description::Value GuiBindableTreeView::ItemSource::GetItemSource()
			{
				return rootNode->GetItemSource();
			}

			void GuiBindableTreeView::ItemSource::SetItemSource(const description::Value& _itemSource)
			{
				rootNode->SetItemSource(_itemSource);
			}

			void GuiBindableTreeView::ItemSource::UpdateBindingProperties(bool updateChildrenProperty)
			{
				vint oldCount = rootNode->GetChildCount();
				if (updateChildrenProperty)
				{
					rootNode->UnprepareChildren();
				}
				vint newCount = rootNode->GetChildCount();
				OnBeforeItemModified(rootNode.Obj(), 0, oldCount, newCount);
				OnAfterItemModified(rootNode.Obj(), 0, oldCount, newCount);
			}

			// ===================== tree::INodeRootProvider =====================

			tree::INodeProvider* GuiBindableTreeView::ItemSource::GetRootNode()
			{
				return rootNode.Obj();
			}

			IDescriptable* GuiBindableTreeView::ItemSource::RequestView(const WString& identifier)
			{
				if(identifier==INodeItemBindingView::Identifier)
				{
					return (INodeItemBindingView*)this;
				}
				else if(identifier==INodeItemPrimaryTextView::Identifier)
				{
					return (INodeItemPrimaryTextView*)this;
				}
				else if(identifier==ITreeViewItemView::Identifier)
				{
					return (ITreeViewItemView*)this;
				}
				else
				{
					return 0;
				}
			}

			void GuiBindableTreeView::ItemSource::ReleaseView(IDescriptable* view)
			{
			}

			// ===================== tree::INodeItemBindingView =====================

			description::Value GuiBindableTreeView::ItemSource::GetBindingValue(tree::INodeProvider* node)
			{
				if (auto itemSourceNode = dynamic_cast<ItemSourceNode*>(node))
				{
					return itemSourceNode->GetItemSource();
				}
				return Value();
			}

			// ===================== tree::INodeItemPrimaryTextView =====================

			WString GuiBindableTreeView::ItemSource::GetPrimaryTextViewText(tree::INodeProvider* node)
			{
				return GetNodeText(node);
			}

			// ===================== tree::ITreeViewItemView =====================

			Ptr<GuiImageData> GuiBindableTreeView::ItemSource::GetNodeImage(tree::INodeProvider* node)
			{
				if (auto itemSourceNode = dynamic_cast<ItemSourceNode*>(node))
				{
					auto value = ReadProperty(itemSourceNode->GetItemSource(), imageProperty);
					return value.GetSharedPtr().Cast<GuiImageData>();
				}
				return nullptr;
			}

			WString GuiBindableTreeView::ItemSource::GetNodeText(tree::INodeProvider* node)
			{
				if (auto itemSourceNode = dynamic_cast<ItemSourceNode*>(node))
				{
					return ReadProperty(itemSourceNode->GetItemSource(), textProperty).GetText();
				}
				return L"";
			}

/***********************************************************************
GuiBindableTreeView
***********************************************************************/

			GuiBindableTreeView::GuiBindableTreeView(IStyleProvider* _styleProvider)
				:GuiVirtualTreeView(_styleProvider, new ItemSource)
			{
				itemSource = dynamic_cast<ItemSource*>(GetNodeRootProvider());

				TextPropertyChanged.SetAssociatedComposition(boundsComposition);
				ImagePropertyChanged.SetAssociatedComposition(boundsComposition);
				ChildrenPropertyChanged.SetAssociatedComposition(boundsComposition);
			}

			GuiBindableTreeView::~GuiBindableTreeView()
			{
			}

			description::Value GuiBindableTreeView::GetItemSource()
			{
				return itemSource->GetItemSource();
			}

			void GuiBindableTreeView::SetItemSource(description::Value _itemSource)
			{
				itemSource->SetItemSource(_itemSource);
			}

			const WString& GuiBindableTreeView::GetTextProperty()
			{
				return itemSource->textProperty;
			}

			void GuiBindableTreeView::SetTextProperty(const WString& value)
			{
				if (itemSource->textProperty != value)
				{
					itemSource->textProperty = value;
					itemSource->UpdateBindingProperties(false);
					TextPropertyChanged.Execute(GetNotifyEventArguments());
				}
			}

			const WString& GuiBindableTreeView::GetImageProperty()
			{
				return itemSource->imageProperty;
			}

			void GuiBindableTreeView::SetImageProperty(const WString& value)
			{
				if (itemSource->imageProperty != value)
				{
					itemSource->imageProperty = value;
					itemSource->UpdateBindingProperties(false);
					ImagePropertyChanged.Execute(GetNotifyEventArguments());
				}
			}

			const WString& GuiBindableTreeView::GetChildrenProperty()
			{
				return itemSource->childrenProperty;
			}

			void GuiBindableTreeView::SetChildrenProperty(const WString& value)
			{
				if (itemSource->childrenProperty != value)
				{
					itemSource->childrenProperty = value;
					itemSource->UpdateBindingProperties(true);
					ChildrenPropertyChanged.Execute(GetNotifyEventArguments());
				}
			}

			description::Value GuiBindableTreeView::GetSelectedItem()
			{
				vint index = GetSelectedItemIndex();
				if (index == -1) return Value();

				Value result;
				if (auto node = nodeItemView->RequestNode(index))
				{
					if (auto itemSourceNode = dynamic_cast<ItemSourceNode*>(node))
					{
						result = itemSourceNode->GetItemSource();
					}
					nodeItemView->ReleaseNode(node);
				}
				return result;
			}

			namespace list
			{
/***********************************************************************
GuiBindableDataColumn
***********************************************************************/

				BindableDataColumn::BindableDataColumn()
					:dataProvider(nullptr)
				{
				}

				BindableDataColumn::~BindableDataColumn()
				{
				}

				void BindableDataColumn::SaveCellData(vint row, IDataEditor* dataEditor)
				{
					if (auto editor = dynamic_cast<GuiBindableDataEditor*>(dataEditor))
					{
						SetCellValue(row, editor->GetEditedCellValue());
					}
					if (commandExecutor)
					{
						commandExecutor->OnDataProviderItemModified(row, 1, 1);
					}
				}

				WString BindableDataColumn::GetCellText(vint row)
				{
					return GetCellValue(row).GetText();
				}

				description::Value BindableDataColumn::GetCellValue(vint row)
				{
					if (dataProvider->itemSource)
					{
						if (0 <= row && row < dataProvider->itemSource->GetCount())
						{
							return ReadProperty(dataProvider->itemSource->Get(row), valueProperty);
						}
					}
					return Value();
				}

				void BindableDataColumn::SetCellValue(vint row, description::Value value)
				{
					if (dataProvider->itemSource)
					{
						if (0 <= row && row < dataProvider->itemSource->GetCount())
						{
							auto rowValue = dataProvider->itemSource->Get(row);
							return WriteProperty(rowValue, valueProperty, value);
						}
					}
				}

				const WString& BindableDataColumn::GetValueProperty()
				{
					return valueProperty;
				}

				void BindableDataColumn::SetValueProperty(const WString& value)
				{
					if (valueProperty != value)
					{
						valueProperty = value;
						if (commandExecutor)
						{
							commandExecutor->OnDataProviderColumnChanged();
						}
						compositions::GuiEventArgs arguments;
						ValuePropertyChanged.Execute(arguments);
					}
				}

				const description::Value& BindableDataColumn::GetViewModelContext()
				{
					return dataProvider->viewModelContext;
				}

/***********************************************************************
GuiBindableDataProvider
***********************************************************************/

				BindableDataProvider::BindableDataProvider(const description::Value& _viewModelContext)
					:viewModelContext(_viewModelContext)
				{
				}

				BindableDataProvider::~BindableDataProvider()
				{
					SetItemSource(nullptr);
				}

				Ptr<description::IValueEnumerable> BindableDataProvider::GetItemSource()
				{
					return itemSource;
				}

				void BindableDataProvider::SetItemSource(Ptr<description::IValueEnumerable> _itemSource)
				{
					vint oldCount = 0;
					if (itemSource)
					{
						oldCount = itemSource->GetCount();
					}
					if (itemChangedEventHandler)
					{
						auto ol = itemSource.Cast<IValueObservableList>();
						ol->ItemChanged.Remove(itemChangedEventHandler);
					}

					itemSource = nullptr;
					itemChangedEventHandler = nullptr;

					if (_itemSource)
					{
						if (auto ol = _itemSource.Cast<IValueObservableList>())
						{
							itemSource = ol;
							itemChangedEventHandler = ol->ItemChanged.Add([this](vint start, vint oldCount, vint newCount)
							{
								commandExecutor->OnDataProviderItemModified(start, oldCount, newCount);
							});
						}
						else if (auto rl = _itemSource.Cast<IValueReadonlyList>())
						{
							itemSource = rl;
						}
						else
						{
							itemSource = IValueList::Create(GetLazyList<Value>(_itemSource));
						}
					}

					commandExecutor->OnDataProviderItemModified(0, oldCount, itemSource ? itemSource->GetCount() : 0);
				}

				vint BindableDataProvider::GetRowCount()
				{
					if (!itemSource) return 0;
					return itemSource->GetCount();
				}

				description::Value BindableDataProvider::GetRowValue(vint row)
				{
					if (itemSource)
					{
						if (0 <= row && row < itemSource->GetCount())
						{
							return itemSource->Get(row);
						}
					}
					return Value();
				}

				bool BindableDataProvider::InsertBindableColumn(vint index, Ptr<BindableDataColumn> column)
				{
					if (InsertColumnInternal(index, column, true))
					{
						column->dataProvider = this;
						return true;
					}
					else
					{
						return false;
					}
				}

				bool BindableDataProvider::AddBindableColumn(Ptr<BindableDataColumn> column)
				{
					if (AddColumnInternal(column, true))
					{
						column->dataProvider = this;
						return true;
					}
					else
					{
						return false;
					}
				}

				bool BindableDataProvider::RemoveBindableColumn(Ptr<BindableDataColumn> column)
				{
					if (RemoveColumnInternal(column, true))
					{
						column->dataProvider = nullptr;
						return true;
					}
					else
					{
						return false;
					}
				}

				bool BindableDataProvider::ClearBindableColumns()
				{
					FOREACH(Ptr<StructuredColummProviderBase>, column, columns)
					{
						column.Cast<BindableDataColumn>()->dataProvider = nullptr;
					}
					return ClearColumnsInternal(true);
				}

				Ptr<BindableDataColumn> BindableDataProvider::GetBindableColumn(vint index)
				{
					if (0 <= index && index < GetColumnCount())
					{
						return columns[index].Cast<BindableDataColumn>();
					}
					else
					{
						return nullptr;
					}
				}

				const description::Value& BindableDataProvider::GetViewModelContext()
				{
					return viewModelContext;
				}
			}

/***********************************************************************
GuiBindableDataGrid
***********************************************************************/

			GuiBindableDataGrid::GuiBindableDataGrid(IStyleProvider* _styleProvider, const description::Value& _viewModelContext)
				:GuiVirtualDataGrid(_styleProvider, new BindableDataProvider(_viewModelContext))
			{
				bindableDataProvider = GetStructuredDataProvider()->GetStructuredDataProvider().Cast<BindableDataProvider>();
			}

			GuiBindableDataGrid::~GuiBindableDataGrid()
			{
			}

			Ptr<description::IValueEnumerable> GuiBindableDataGrid::GetItemSource()
			{
				return bindableDataProvider->GetItemSource();
			}

			void GuiBindableDataGrid::SetItemSource(Ptr<description::IValueEnumerable> _itemSource)
			{
				bindableDataProvider->SetItemSource(_itemSource);
			}

			bool GuiBindableDataGrid::InsertBindableColumn(vint index, Ptr<list::BindableDataColumn> column)
			{
				return bindableDataProvider->InsertBindableColumn(index, column);
			}

			bool GuiBindableDataGrid::AddBindableColumn(Ptr<list::BindableDataColumn> column)
			{
				return bindableDataProvider->AddBindableColumn(column);
			}

			bool GuiBindableDataGrid::RemoveBindableColumn(Ptr<list::BindableDataColumn> column)
			{
				return bindableDataProvider->RemoveBindableColumn(column);
			}

			bool GuiBindableDataGrid::ClearBindableColumns()
			{
				return bindableDataProvider->ClearBindableColumns();
			}

			Ptr<list::BindableDataColumn> GuiBindableDataGrid::GetBindableColumn(vint index)
			{
				return bindableDataProvider->GetBindableColumn(index);
			}

			description::Value GuiBindableDataGrid::GetSelectedRowValue()
			{
				return bindableDataProvider->GetRowValue(GetSelectedCell().row);
			}

			description::Value GuiBindableDataGrid::GetSelectedCellValue()
			{
				auto cell = GetSelectedCell();
				auto column = GetBindableColumn(cell.column);
				if (column)
				{
					return column->GetCellValue(cell.row);
				}
				return Value();
			}
		}
	}
}