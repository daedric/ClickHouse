#include <DB/Parsers/ASTTablesInSelectQuery.h>

namespace DB
{

#define CLONE(member) \
do \
{ \
	if (member) \
	{ \
		res->member = std::static_pointer_cast<decltype(res->member)::element_type>(member->clone()); \
		res->children.push_back(res->member); \
	} \
} \
while (0)


ASTPtr ASTTableExpression::clone() const
{
	auto res = std::make_shared<ASTTableExpression>(*this);
	res->children.clear();

	CLONE(database_and_table_name);
	CLONE(table_function);
	CLONE(subquery);
	CLONE(sample_size);
	CLONE(sample_offset);

	return res;
}

ASTPtr ASTTableJoin::clone() const
{
	auto res = std::make_shared<ASTTableJoin>(*this);
	res->children.clear();

	CLONE(using_expression_list);
	CLONE(on_expression);

	return res;
}

ASTPtr ASTArrayJoin::clone() const
{
	auto res = std::make_shared<ASTArrayJoin>(*this);
	res->children.clear();

	CLONE(expression_list);

	return res;
}

ASTPtr ASTTablesInSelectQueryElement::clone() const
{
	auto res = std::make_shared<ASTTablesInSelectQueryElement>(*this);
	res->children.clear();

	CLONE(table_join);
	CLONE(table_expression);
	CLONE(array_join);

	return res;
}

ASTPtr ASTTablesInSelectQuery::clone() const
{
	const auto res = std::make_shared<ASTTablesInSelectQuery>(*this);
	res->children.clear();

	for (const auto & child : children)
		res->children.emplace_back(child->clone());

	return res;
}

#undef CLONE


void ASTTableExpression::formatImpl(const FormatSettings & settings, FormatState & state, FormatStateStacked frame) const
{
	std::string indent_str = settings.one_line ? "" : std::string(4 * frame.indent, ' ');

	if (database_and_table_name)
	{
		database_and_table_name->formatImpl(settings, state, frame);
	}
	else if (table_function)
	{
		table_function->formatImpl(settings, state, frame);
	}
	else if (subquery)
	{
		subquery->formatImpl(settings, state, frame);
	}

	if (final)
	{
		settings.ostr << (settings.hilite ? hilite_keyword : "") << settings.nl_or_ws << indent_str
			<< "FINAL" << (settings.hilite ? hilite_none : "");
	}

	if (sample_size)
	{
		settings.ostr << (settings.hilite ? hilite_keyword : "") << settings.nl_or_ws << indent_str
			<< "SAMPLE " << (settings.hilite ? hilite_none : "");
		sample_size->formatImpl(settings, state, frame);

		if (sample_offset)
		{
			settings.ostr << (settings.hilite ? hilite_keyword : "") << ' '
				<< "OFFSET " << (settings.hilite ? hilite_none : "");
			sample_offset->formatImpl(settings, state, frame);
		}
	}
}


void ASTTableJoin::formatImplBeforeTable(const FormatSettings & settings, FormatState & state, FormatStateStacked frame) const
{
	settings.ostr << (settings.hilite ? hilite_keyword : "");

	if (locality == Locality::Global)
		settings.ostr << "GLOBAL ";

	if (kind != Kind::Cross && kind != Kind::Comma)
		settings.ostr << (strictness == Strictness::Any ? "ANY " : "ALL ");

	settings.ostr <<
		  (kind == Kind::Inner ? "INNER"
		: (kind == Kind::Left ? "LEFT"
		: (kind == Kind::Right ? "RIGHT"
		: (kind == Kind::Cross ? "CROSS"
		: "FULL OUTER"))));

	settings.ostr << " JOIN "
		<< (settings.hilite ? hilite_none : "");
}


void ASTTableJoin::formatImplAfterTable(const FormatSettings & settings, FormatState & state, FormatStateStacked frame) const
{
	frame.need_parens = false;

	if (using_expression_list)
	{
		settings.ostr << (settings.hilite ? hilite_keyword : "") << " USING " << (settings.hilite ? hilite_none : "");
		settings.ostr << "(";
		using_expression_list->formatImpl(settings, state, frame);
		settings.ostr << ")";
	}
	else if (on_expression)
	{
		settings.ostr << (settings.hilite ? hilite_keyword : "") << " ON " << (settings.hilite ? hilite_none : "");
		on_expression->formatImpl(settings, state, frame);
	}
}


void ASTTableJoin::formatImpl(const FormatSettings & settings, FormatState & state, FormatStateStacked frame) const
{
	formatImplBeforeTable(settings, state, frame);
	settings.ostr << " ... ";
	formatImplAfterTable(settings, state, frame);
}


void ASTArrayJoin::formatImpl(const FormatSettings & settings, FormatState & state, FormatStateStacked frame) const
{
	settings.ostr << (settings.hilite ? hilite_keyword : "")
		<< (kind == Kind::Left ? "LEFT " : "") << "ARRAY JOIN " << (settings.hilite ? hilite_none : "");

	settings.one_line
		? expression_list->formatImpl(settings, state, frame)
		: typeid_cast<const ASTExpressionList &>(*expression_list).formatImplMultiline(settings, state, frame);
}


void ASTTablesInSelectQueryElement::formatImpl(const FormatSettings & settings, FormatState & state, FormatStateStacked frame) const
{
	if (table_expression)
	{
		if (table_join)
			table_join->formatImplBeforeTable(settings, state, frame);

		settings.ostr << " ";
		table_expression->formatImpl(settings, state, frame);

		settings.ostr << " ";
		if (table_join)
			table_join->formatImplAfterTable(settings, state, frame);
	}
	else if (array_join)
	{
		array_join->formatImpl(settings, state, frame);
	}
}


void ASTTablesInSelectQuery::formatImpl(const FormatSettings & settings, FormatState & state, FormatStateStacked frame) const
{
	std::string indent_str = settings.one_line ? "" : std::string(4 * frame.indent, ' ');

	for (ASTs::const_iterator it = children.begin(); it != children.end(); ++it)
	{
		if (it != children.begin())
			settings.ostr << settings.nl_or_ws << indent_str;

		(*it)->formatImpl(settings, state, frame);
	}
}

}