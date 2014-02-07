package org.genivi.commonapi.someip.generator;

import java.util.ArrayList;
import java.util.List;

public class ListBuilder<T> {
	private List<T> list = new ArrayList<T>();

	public ListBuilder<T> concat(List<T> other) {
		this.list.addAll(other);
		return this;
	}

	public static List join(List list1, List list2) {
		ListBuilder builder = new ListBuilder();
		builder.concat(list1);
		builder.concat(list2);
		return builder.list;
	}
	
	public static List join(List list1) {
		ListBuilder builder = new ListBuilder();
		builder.concat(list1);
		return builder.list;
	}
	
	public static List join(List list1, List list2, List list3) {
		ListBuilder builder = new ListBuilder();
		builder.concat(list1);
		builder.concat(list2);
		builder.concat(list3);
		return builder.list;
	}

	public static List join(List list1, Object list2, List list3) {
		ListBuilder builder = new ListBuilder();
		builder.concat(list1);
		builder.list.add(list2);
		builder.concat(list3);
		return builder.list;
	}
	
	public static List join(List list1, Object list2) {
		ListBuilder builder = new ListBuilder();
		builder.concat(list1);
		builder.list.add(list2);
		return builder.list;
	}

	public List<T> getAll() {
		return this.list;
	}

	public static void test() {
		ListBuilder<String> build = new ListBuilder<String>();
		build.concat(new ArrayList<String>());
		join(new ArrayList<String>(), new ArrayList<String>());
	}

}
